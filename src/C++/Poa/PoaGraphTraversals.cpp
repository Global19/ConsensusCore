// Author: David Alexander

#include <limits>

#include <ConsensusCore/Matrix/VectorL.hpp>
#include <ConsensusCore/Poa/PoaGraph.hpp>
#include <ConsensusCore/Utils.hpp>

#include <boost/graph/topological_sort.hpp>

#include "PoaGraphImpl.hpp"

namespace ConsensusCore {
namespace detail {

std::string sequenceAlongPath(const BoostGraph&, const VertexInfoMap& vertexInfoMap,
                              const std::vector<VD>& path)
{
    std::stringstream ss;
    foreach (VD v, path) {
        ss << vertexInfoMap[v].Base;
    }
    return ss.str();
}

void PoaGraphImpl::tagSpan(VD start, VD end)
{
    // cout << "Tagging span " << start << " to " << end << endl;
    std::list<VD> sortedVertices(num_vertices(g_));
    topological_sort(g_, sortedVertices.rbegin());
    bool spanning = false;
    foreach (VD v, sortedVertices) {
        if (v == start) {
            spanning = true;
        }
        if (v == end) {
            break;
        }
        if (spanning) {
            vertexInfoMap_[v].SpanningReads++;
        }
    }
}

std::vector<VD> PoaGraphImpl::consensusPath(AlignMode mode, int minCoverage) const
{
    // Pat's note on the approach here:
    //
    // "A node gets a score of NumReads if all reads go through
    //  it, and a score of -NumReads if no reads go through it The
    //  shift of -0.0001 breaks ties in favor of skipping
    //  half-full nodes.  In the 2 reads case this will get rid of
    //  insertions which are the more common error."
    //
    // The interpretation of minCoverage (which is applicable only
    // for LOCAL, SEMIGLOBAL modes) is that it represents
    // application-specific knowledge of the basal coverage level
    // of reads in the template, such that if a node is contained
    // in fewer than minCoverage reads, it will be penalized
    // against inclusion in the consensus.
    int totalReads = NumReads();

    std::list<VD> path;
    std::list<VD> sortedVertices(num_vertices(g_));
    topological_sort(g_, sortedVertices.rbegin());
    unordered_map<VD, VD> bestPrevVertex;

    // ignore ^ and $
    // TODO(dalexander): find a cleaner way to do this
    vertexInfoMap_[sortedVertices.front()].ReachingScore = 0;
    sortedVertices.pop_back();
    sortedVertices.pop_front();

    VD bestVertex = null_vertex;
    float bestReachingScore = -FLT_MAX;
    foreach (VD v, sortedVertices) {
        PoaNode& vInfo = vertexInfoMap_[v];
        int containingReads = vInfo.Reads;
        int spanningReads = vInfo.SpanningReads;
        float score =
            (mode != GLOBAL)
                ? (2 * containingReads - 1 * std::max(spanningReads, minCoverage) - 0.0001f)
                : (2 * containingReads - 1 * totalReads - 0.0001f);
        vInfo.Score = score;
        vInfo.ReachingScore = score;
        bestPrevVertex[v] = null_vertex;
        foreach (ED e, inEdges(v, g_)) {
            VD sourceVertex = source(e, g_);
            float rsc = score + vertexInfoMap_[sourceVertex].ReachingScore;
            if (rsc > vInfo.ReachingScore) {
                vInfo.ReachingScore = rsc;
                bestPrevVertex[v] = sourceVertex;
            }
            if (rsc > bestReachingScore) {
                bestVertex = v;
                bestReachingScore = rsc;
            }
        }
    }
    assert(bestVertex != null_vertex);

    // trace back from best-scoring vertex
    VD v = bestVertex;
    while (v != null_vertex) {
        path.push_front(v);
        v = bestPrevVertex[v];
    }
    return std::vector<VD>(path.begin(), path.end());
}

void PoaGraphImpl::threadFirstRead(std::string sequence, std::vector<Vertex>* outputPath)
{
    // first sequence in the alignment
    VD u = null_vertex, v;
    VD startSpanVertex = null_vertex, endSpanVertex;
    int readPos = 0;

    if (outputPath) {
        outputPath->clear();
    }

    foreach (char base, sequence) {
        v = addVertex(base);
        if (outputPath) {
            outputPath->push_back(externalize(v));
        }
        if (readPos == 0) {
            add_edge(enterVertex_, v, g_);
            startSpanVertex = v;
        } else {
            add_edge(u, v, g_);
        }
        u = v;
        readPos++;
    }
    assert(startSpanVertex != null_vertex);
    assert(u != null_vertex);
    endSpanVertex = u;
    add_edge(u, exitVertex_, g_);  // terminus -> $
    tagSpan(startSpanVertex, endSpanVertex);
}

void PoaGraphImpl::tracebackAndThread(std::string sequence,
                                      const AlignmentColumnMap& alignmentColumnForVertex,
                                      AlignMode alignMode, std::vector<Vertex>* outputPath)
{
    const int I = sequence.length();

    // perform traceback from (I,$), threading the new sequence into the graph as
    // we go.
    int i = I;
    const AlignmentColumn* curCol;
    VD v = null_vertex, forkVertex = null_vertex;
    VD u = exitVertex_;
    VD startSpanVertex;
    VD endSpanVertex = alignmentColumnForVertex.at(exitVertex_)->PreviousVertex[I];

    if (outputPath) {
        outputPath->resize(I);
        std::fill(outputPath->begin(), outputPath->end(), std::numeric_limits<size_t>::max());
    }

#define READPOS (i - 1)
#define VERTEX_ON_PATH(readPos, v)                 \
    if (outputPath) {                              \
        (*outputPath)[(readPos)] = externalize(v); \
    }

    while (!(u == enterVertex_ && i == 0)) {
        // u -> v
        // u: current vertex
        // v: vertex last visited in traceback (could be == u)
        // forkVertex: the vertex that will be the target of a new edge
        curCol = alignmentColumnForVertex.at(u);
        assert(curCol != NULL);
        PoaNode& curNodeInfo = vertexInfoMap_[u];
        VD prevVertex = curCol->PreviousVertex[i];
        MoveType reachingMove = curCol->ReachingMove[i];

        if (reachingMove == StartMove) {
            assert(v != null_vertex);

            if (forkVertex == null_vertex) {
                forkVertex = v;
            }
            // In local model thread read bases, adjusting i (should stop at 0)
            while (i > 0) {
                assert(alignMode == LOCAL);
                VD newForkVertex = addVertex(sequence[READPOS]);
                add_edge(newForkVertex, forkVertex, g_);
                VERTEX_ON_PATH(READPOS, newForkVertex);
                forkVertex = newForkVertex;
                i--;
            }
        } else if (reachingMove == EndMove) {
            assert(forkVertex == null_vertex && u == exitVertex_ && v == null_vertex);

            forkVertex = exitVertex_;

            if (alignMode == LOCAL) {
                // Find the row # we are coming from, walk
                // back to there, threading read bases onto
                // graph via forkVertex, adjusting i.
                const AlignmentColumn* prevCol = alignmentColumnForVertex.at(prevVertex);
                int prevRow = ArgMax(prevCol->Score);

                while (i > prevRow) {
                    VD newForkVertex = addVertex(sequence[READPOS]);
                    add_edge(newForkVertex, forkVertex, g_);
                    VERTEX_ON_PATH(READPOS, newForkVertex);
                    forkVertex = newForkVertex;
                    i--;
                }
            }
        } else if (reachingMove == MatchMove) {
            VERTEX_ON_PATH(READPOS, u);
            // if there is an extant forkVertex, join it
            if (forkVertex != null_vertex) {
                add_edge(u, forkVertex, g_);
                forkVertex = null_vertex;
            }
            // add to existing node
            curNodeInfo.Reads++;
            i--;
        } else if (reachingMove == DeleteMove) {
            if (forkVertex == null_vertex) {
                forkVertex = v;
            }
        } else if (reachingMove == ExtraMove || reachingMove == MismatchMove) {
            // begin a new arc with this read base
            VD newForkVertex = addVertex(sequence[READPOS]);
            if (forkVertex == null_vertex) {
                forkVertex = v;
            }
            add_edge(newForkVertex, forkVertex, g_);
            VERTEX_ON_PATH(READPOS, newForkVertex);
            forkVertex = newForkVertex;
            i--;
        } else {
            ShouldNotReachHere();
        }

        v = u;
        u = prevVertex;
    }
    startSpanVertex = v;
    if (startSpanVertex != exitVertex_) {
        tagSpan(startSpanVertex, endSpanVertex);
    }

    // if there is an extant forkVertex, join it to enterVertex
    if (forkVertex != null_vertex) {
        add_edge(enterVertex_, forkVertex, g_);
        forkVertex = null_vertex;
    }

    // all filled in?
    assert(outputPath == NULL ||
           std::find(outputPath->begin(), outputPath->end(), std::numeric_limits<size_t>::max()) ==
               outputPath->end());

#undef READPOS
#undef VERTEX_ON_PATH
}

static boost::unordered_set<VD> childVertices(VD v, const BoostGraph& g)
{
    boost::unordered_set<VD> result;
    foreach (ED e, out_edges(v, g)) {
        result.insert(target(e, g));
    }
    return result;
}

static boost::unordered_set<VD> parentVertices(VD v, const BoostGraph& g)
{
    boost::unordered_set<VD> result;
    foreach (ED e, in_edges(v, g)) {
        result.insert(source(e, g));
    }
    return result;
}

vector<ScoredMutation>* PoaGraphImpl::findPossibleVariants(
    const std::vector<Vertex>& bestPath) const
{
    std::vector<VD> bestPath_ = internalizePath(bestPath);

    // Return value will be deallocated by PoaConsensus destructor.
    vector<ScoredMutation>* variants = new vector<ScoredMutation>();

    for (int i = 2; i < static_cast<int>(bestPath_.size()) - 2; i++)  // NOLINT
    {
        VD v = bestPath_[i];
        boost::unordered_set<VD> children = childVertices(v, g_);

        // Look for a direct edge from the current node to the node
        // two spaces down---suggesting a deletion with respect to
        // the consensus sequence.
        if (children.find(bestPath_[i + 2]) != children.end()) {
            float score = -vertexInfoMap_[bestPath_[i + 1]].Score;
            variants->push_back(Mutation(DELETION, i + 1, '-').WithScore(score));
        }

        // Look for a child node that connects immediately back to i + 1.
        // This indicates we should try inserting the base at i + 1.

        // Parents of (i + 1)
        boost::unordered_set<VD> lookBack = parentVertices(bestPath_[i + 1], g_);

        // (We could do this in STL using std::set sorted on score, which would then
        // provide an intersection mechanism (in <algorithm>) but that actually ends
        // up being more code.  Sad.)
        float bestInsertScore = -FLT_MAX;
        VD bestInsertVertex = null_vertex;

        foreach (VD vi, children) {
            boost::unordered_set<VD>::iterator found = lookBack.find(vi);
            if (found != lookBack.end()) {
                float score = vertexInfoMap_[*found].Score;
                if (score > bestInsertScore) {
                    bestInsertScore = score;
                    bestInsertVertex = *found;
                }
            }
        }

        if (bestInsertVertex != null_vertex) {
            char base = vertexInfoMap_[bestInsertVertex].Base;
            variants->push_back(Mutation(INSERTION, i + 1, base).WithScore(bestInsertScore));
        }

        // Look for a child node not in the consensus that connects immediately
        // to i + 2.  This indicates we should try mismatching the base i + 1.

        // Parents of (i + 2)
        lookBack = parentVertices(bestPath_[i + 2], g_);

        float bestMismatchScore = -FLT_MAX;
        VD bestMismatchVertex = null_vertex;

        foreach (VD vi, children) {
            if (vi == bestPath_[i + 1]) continue;

            boost::unordered_set<VD>::iterator found = lookBack.find(vi);
            if (found != lookBack.end()) {
                float score = vertexInfoMap_[*found].Score;
                if (score > bestMismatchScore) {
                    bestMismatchScore = score;
                    bestMismatchVertex = *found;
                }
            }
        }

        if (bestMismatchVertex != null_vertex) {
            // TODO(dalexander): As implemented (compatibility), this returns
            // the score of the mismatch node. I think it should return the score
            // difference, no?
            char base = vertexInfoMap_[bestMismatchVertex].Base;
            variants->push_back(Mutation(SUBSTITUTION, i + 1, base).WithScore(bestMismatchScore));
        }
    }
    return variants;
}
}
}  // ConsensusCore::detail
