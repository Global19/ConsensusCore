// Author: David Alexander

#include <ConsensusCore/Align/PairwiseAlignment.hpp>

#include <algorithm>
#include <boost/numeric/ublas/matrix.hpp>
#include <string>
#include <vector>

#include <ConsensusCore/Sequence.hpp>
#include <ConsensusCore/Types.hpp>
#include <ConsensusCore/Utils.hpp>

namespace ConsensusCore {

std::string PairwiseAlignment::Target() const { return target_; }

std::string PairwiseAlignment::Query() const { return query_; }

float PairwiseAlignment::Accuracy() const { return (static_cast<float>(Matches())) / Length(); }

std::string PairwiseAlignment::Transcript() const { return transcript_; }

int PairwiseAlignment::Matches() const
{
    return std::count(transcript_.begin(), transcript_.end(), 'M');
}

int PairwiseAlignment::Errors() const { return Length() - Matches(); }

int PairwiseAlignment::Mismatches() const
{
    return std::count(transcript_.begin(), transcript_.end(), 'R');
}

int PairwiseAlignment::Insertions() const
{
    return std::count(transcript_.begin(), transcript_.end(), 'I');
}

int PairwiseAlignment::Deletions() const
{
    return std::count(transcript_.begin(), transcript_.end(), 'D');
}

int PairwiseAlignment::Length() const { return target_.length(); }

PairwiseAlignment::PairwiseAlignment(const std::string& target, const std::string& query)
    : target_(target), query_(query), transcript_(target_.length(), 'Z')
{
    if (target_.length() != query_.length()) {
        throw InvalidInputError();
    }
    for (unsigned int i = 0; i < target_.length(); i++) {
        char t = target_[i];
        char q = query_[i];
        char tr;

        if (t == '-' && q == '-') {
            throw InvalidInputError();
        } else if (t == q) {
            tr = 'M';
        } else if (t == '-') {
            tr = 'I';
        } else if (q == '-') {
            tr = 'D';
        } else {
            tr = 'R';
        }  // NOLINT

        transcript_[i] = tr;
    }
}

PairwiseAlignment* Align(const std::string& target, const std::string& query, int* score,
                         AlignConfig config)
{
    using boost::numeric::ublas::matrix;

    const AlignParams& params = config.Params;
    if (config.Mode != GLOBAL) {
        throw UnsupportedFeatureError("Only GLOBAL alignment supported at present");
    }

    int I = query.length();
    int J = target.length();
    matrix<int> Score(I + 1, J + 1);

    Score(0, 0) = 0;
    for (int i = 1; i <= I; i++) {
        Score(i, 0) = i * params.Insert;
    }
    for (int j = 1; j <= J; j++) {
        Score(0, j) = j * params.Delete;
    }
    for (int i = 1; i <= I; i++) {
        for (int j = 1; j <= J; j++) {
            bool isMatch = (query[i - 1] == target[j - 1]);
            Score(i, j) = Max3(Score(i - 1, j - 1) + (isMatch ? params.Match : params.Mismatch),
                               Score(i - 1, j) + params.Insert, Score(i, j - 1) + params.Delete);
        }
    }
    if (score != NULL) {
        *score = Score(I, J);
    }

    // Traceback, build up reversed aligned query, aligned target
    std::string raQuery, raTarget;
    int i = I, j = J;
    while (i > 0 || j > 0) {
        int move;
        if (i == 0) {
            move = 2;  // only deletion is possible
        } else if (j == 0) {
            move = 1;  // only insertion is possible
        } else {
            bool isMatch = (query[i - 1] == target[j - 1]);
            move = ArgMax3(Score(i - 1, j - 1) + (isMatch ? params.Match : params.Mismatch),
                           Score(i - 1, j) + params.Insert, Score(i, j - 1) + params.Delete);
        }
        // Incorporate:
        if (move == 0) {
            i--;
            j--;
            raQuery.push_back(query[i]);
            raTarget.push_back(target[j]);
        }
        // Insert:
        else if (move == 1) {
            i--;
            raQuery.push_back(query[i]);
            raTarget.push_back('-');
        }
        // Delete:
        else if (move == 2) {
            j--;
            raQuery.push_back('-');
            raTarget.push_back(target[j]);
        }
    }

    return new PairwiseAlignment(Reverse(raTarget), Reverse(raQuery));
}

PairwiseAlignment* Align(const std::string& target, const std::string& query, AlignConfig config)
{
    return Align(target, query, NULL, config);
}

//
//  Code for lifting target coordinates into query coordinates.
//

static bool addsToTarget(char transcriptChar)
{
    return (transcriptChar == 'M' || transcriptChar == 'R' || transcriptChar == 'D');
}

static int targetLength(const std::string& alignmentTranscript)
{
    return std::count_if(alignmentTranscript.begin(), alignmentTranscript.end(), addsToTarget);
}

#ifndef NDEBUG
static bool addsToQuery(char transcriptChar)
{
    return (transcriptChar == 'M' || transcriptChar == 'R' || transcriptChar == 'I');
}

static int queryLength(const std::string& alignmentTranscript)
{
    return std::count_if(alignmentTranscript.begin(), alignmentTranscript.end(), addsToQuery);
}
#endif  // !NDEBUG

// TargetPositionsInQuery:
// * Returns a vector of targetLength(transcript) + 1, which,
//   roughly speaking, indicates the positions in the query string of the
//   the characters in the target, as induced by an alignment with the
//   given transcript string.
// * More precisely, given an alignment (T, Q, X)  (x=transcript),
//   letting T[s, e) denote any slice of T,
//    - [s',e') denote the subslice of indices of Q aligned to T[s, e),
//    - ntp = NewTargetPositions(X)
//   we have
//      [s', e') = [ntp(s), ntp(e))
//
// * Ex:
//     MMM -> 0123
//     DMM -> 0012,  MMD -> 0122, MDM -> 0112
//     IMM -> 123,   MMI -> 013,  MIM -> 023
//     MRM, MIDM, MDIM -> 0123
std::vector<int> TargetToQueryPositions(const std::string& transcript)
{
    std::vector<int> ntp;
    ntp.reserve(targetLength(transcript) + 1);

    int targetPos = 0;
    int queryPos = 0;
    foreach (char c, transcript) {
        if (c == 'M' || c == 'R') {
            ntp.push_back(queryPos);
            targetPos++;
            queryPos++;
        } else if (c == 'D') {
            ntp.push_back(queryPos);
            targetPos++;
        } else if (c == 'I') {
            queryPos++;
        } else {
            ShouldNotReachHere();
        }
    }
    ntp.push_back(queryPos);

    assert(static_cast<int>(ntp.size()) == targetLength(transcript) + 1);
    assert(ntp[targetLength(transcript)] == queryLength(transcript));
    return ntp;
}

std::vector<int> TargetToQueryPositions(const PairwiseAlignment& aln)
{
    return TargetToQueryPositions(aln.Transcript());
}

// Build the alignment given the unaligned sequences and the transcript
// Returns NULL if transcript does not map unalnTarget into unalnQuery.
PairwiseAlignment* PairwiseAlignment::FromTranscript(const std::string& transcript,
                                                     const std::string& unalnTarget,
                                                     const std::string& unalnQuery)
{
    std::string alnTarget;
    std::string alnQuery;
    int tPos, qPos;
    int tLen, qLen;

    tLen = unalnTarget.length();
    qLen = unalnQuery.length();
    tPos = 0;
    qPos = 0;
    foreach (char x, transcript) {
        if (tPos > tLen || qPos > qLen) {
            return NULL;
        }

        char t = (tPos < tLen ? unalnTarget[tPos] : '\0');
        char q = (qPos < qLen ? unalnQuery[qPos] : '\0');

        switch (x) {
            case 'M':
                if (t != q) {
                    return NULL;
                }
                alnTarget.push_back(t);
                alnQuery.push_back(q);
                tPos++;
                qPos++;
                break;
            case 'R':
                if (t == q) {
                    return NULL;
                }
                alnTarget.push_back(t);
                alnQuery.push_back(q);
                tPos++;
                qPos++;
                break;
            case 'I':
                alnTarget.push_back('-');
                alnQuery.push_back(q);
                qPos++;
                break;
            case 'D':
                alnTarget.push_back(t);
                alnQuery.push_back('-');
                tPos++;
                break;
            default:
                return NULL;
        }
    }
    // Didn't consume all of one of the strings
    if (tPos != tLen || qPos != qLen) {
        return NULL;
    }

    // Provide another constructor to inject transcript?  Calculate transcript on
    // the fly?
    return new PairwiseAlignment(alnTarget, alnQuery);
}
}
