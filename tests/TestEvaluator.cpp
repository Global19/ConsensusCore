// Copyright (c) 2011-2014, Pacific Biosciences of California, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//  * Neither the name of Pacific Biosciences nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY PACIFIC
// BIOSCIENCES AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL PACIFIC BIOSCIENCES OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

// Author: David Alexander

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <pacbio/consensus/Integrator.h>
#include <pacbio/consensus/Mutation.h>

using std::string;
using std::vector;
using std::cout;
using std::endl;

using namespace PacBio::Consensus;  // NOLINT

using ::testing::UnorderedElementsAreArray;

namespace {

vector<Mutation> Mutations(const string& tpl, const size_t start, const size_t end)
{
    constexpr auto bases = "ACGT";

    vector<Mutation> result;

    for (size_t i = start; i < end; ++i) {
        for (size_t j = 0; j < 4; ++j)
            result.push_back(Mutation(MutationType::INSERTION, i, bases[j]));

        result.push_back(Mutation(MutationType::DELETION, i));

        for (size_t j = 0; j < 4; ++j)
            if (bases[j] != tpl[i])
                result.push_back(Mutation(MutationType::SUBSTITUTION, i, bases[j]));
    }

    for (size_t j = 0; j < 4; ++j)
        result.push_back(Mutation(MutationType::INSERTION, tpl.length(), bases[j]));

    return result;
}

vector<Mutation> Mutations(const string& tpl) { return Mutations(tpl, 0, tpl.length()); }
}  // namespace anonymous

const double prec = 0.001;  // alpha/beta mismatch tolerance
const SNR snr(10, 7, 5, 11);
const string mdl = "P6/C4";
const string longTpl =
    "GGGCGGCGACCTCGCGGGTTTTCGCTATTTATGAAAATTTTCCGGTTTAAGGCGTTTCCGTTCTTCTTCGTCAT"
    "AACTTAATGTTTTTATTTAAAATACCCTCTGAAAAGAAAGGAAACGACAGGTGCTGAAAGCGAGCTTTTTGGCC"
    "TCTGTCGTTTCCTTTCTCTGTTTTTGTCCGTGGAATGAACAATGGAAGTCAACAAAAAGCAGCTGGCTGACATT"
    "TTCGGTGCGAGTATCCGTACCATTCAGAACTGGCAGGAACAGGGAATGCCCGTTCTGCGAGGCGGTGGCAAGGG"
    "TAATGAGGTGCTTTATGACTCTGCCGCCGTCATAAAATGGTATGCCGAAAGGGATGCTGAAATTGAGAACGAAA"
    "AGCTGCGCCGGGAGGTTGAAGAACTGCGGCAGGCCAGCGAGGCAGATCTCCAGCCAGGAACTATTGAGTACGAA"
    "CGCCATCGACTTACGCGTGCGCAGGCCGACGCACAGGAACTGAAGAATGCCAGAGACTCCGCTGAAGTGGTGGA"
    "AACCGCATTCTGTACTTTCGTGCTGTCGCGGATCGCAGGTGAAATTGCCAGTATTCTCGACGGGCTCCCCCTGT"
    "CGGTGCAGCGGCGTTTTCCGGAACTGGAAAACCGACATGTTGATTTCCTGAAACGGGATATCATCAAAGCCATG"
    "AACAAAGCAGCCGCGCTGGATGAACTGATACCGGGGTTGCTGAGTGAATATATCGAACAGTCAGGTTAACAGGC"
    "TGCGGCATTTTGTCCGCGCCGGGCTTCGCTCACTGTTCAGGCCGGAGCCACAGACCGCCGTTGAATGGGCGGAT"
    "GCTAATTACTATCTCCCGAAAGAATC";
const string longRead =
    "GGGCGGCGACCTCGCGGGTTTTCGCTATTTCTGAAAATTTTCCGGTTTAAGGCGTTTCCGTTCTTCTTCGTCAT"
    "AACTTAATGTTTTTATTTAAAATACCCTCTGAAAAGAAAGGAAACGACAGGTGCTGAAAGCGAGCTTTTTGGCC"
    "TCTGTCGTTTCCTTTCTCTGTTTTTGTCCGTGGAATGAACAATGGAAGTCAACAAAAAGCAGCTGGCTGACATT"
    "TTCGGTGGAGTATCCGTACCATTCAGAACTGGCAGGACAGGGAATGCCCGTTCTGCGAGGCGGTGGCAAGGGTA"
    "ATGAGGTGCTTTATGACTCTGCCGCCGTCATAAAATGGTATGCCGAAAGGGATGCTGAAATTGAGAACGAATAG"
    "CTGCGCCGGGAGGTTGAAGAACTGCGGCAGGCCAGCGAGGCAGATCTCCAGCCAGGAACTATTGAGTACGAACG"
    "CCATCGACTTACGCGTGCGCAGGCCGACGCACAGGAACTGAAGAATGCCAGAGACTCCGCTGAAGTGGTGGAAA"
    "CCGCATTCCCCTGTACTTTCGTGCTGTCGCGGATCGCAGGTGAAATTGCCAGTATTCTCGACGGGCTCCCCCTG"
    "TCGGTGCAGCGGCGTTTTCCGGAACTGGAAAACCGACATGTTGATTTCCTGAAACGGGATATCATCAAAGCCAT"
    "GAACAAAGCAGCCGCGCTGGATGAACTGATACCGGGGTTGCTGAGTGAATATATCGAACAGTCAGGTTAACAGG"
    "CTGCGGCATTTTGTCCGCGCCGGGCTTCGCTCACTGTTCAGGCCGGAGCCACAGACCGCCGTTGAACGGATGCT"
    "AATTACTATCTCCCGAAAGAATC";
const IntegratorConfig cfg;

TEST(EvaluatorTest, TestLongTemplate)
{
    MonoMolecularIntegrator ai(longTpl, cfg, snr, mdl);
    ai.AddRead(MappedRead(Read("N/A", longRead, mdl), StrandEnum::FORWARD, 0, longTpl.length()));
    EXPECT_NEAR(-148.92614949338801011, ai.LL(), prec);
}

TEST(EvaluatorTest, TestLongTemplateTiming)
{
    const size_t nsamp = 200;
    MonoMolecularIntegrator ai(longTpl, cfg, snr, mdl);
    const auto stime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < nsamp; ++i)
        ai.AddRead(
            MappedRead(Read("N/A", longRead, mdl), StrandEnum::FORWARD, 0, longTpl.length()));
    const auto etime = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(etime - stime).count();
    EXPECT_LT(duration / nsamp, 1500);
}

std::string RandomDNA(const size_t n, std::mt19937* const gen)
{
    constexpr auto bases = "ACGT";
    string result(n, 'A');
    std::uniform_int_distribution<size_t> rand(0, 3);

    for (size_t i = 0; i < n; ++i)
        result[i] = bases[rand(*gen)];

    return result;
}

std::string Mutate(const std::string& tpl, const size_t nmut, std::mt19937* const gen)
{
    if (nmut == 0) return tpl;

    std::vector<Mutation> muts;
    std::uniform_int_distribution<size_t> rand(0, tpl.length() - 1);
    std::set<size_t> sites;

    while (sites.size() < nmut)
        sites.insert(rand(*gen));

    for (const size_t site : sites) {
        vector<Mutation> possible = Mutations(tpl, site, site + 1);
        std::uniform_int_distribution<size_t> rand2(0, possible.size() - 1);
        muts.push_back(possible[rand2(*gen)]);
    }

    return ApplyMutations(tpl, &muts);
}

template <typename F, typename G>
void MutationEquivalence(const size_t nsamp, const size_t nmut, const F& makeIntegrator,
                         const G& addRead)
{
    // std::random_device rd;
    std::mt19937 gen(42);
    // increase the floor by nmut because we do not support templates with lt 3
    // bases
    std::uniform_int_distribution<size_t> rand(3 + nmut, 30);

    // count how bad we do
    size_t ntests = 0;
    size_t nerror = 0;

    for (size_t i = 0; i < nsamp; ++i) {
        const string tpl           = RandomDNA(rand(gen), &gen);
        vector<Mutation> mutations = Mutations(tpl);
        for (const auto& mut : mutations) {
            vector<Mutation> muts{mut};
            const string app  = ApplyMutations(tpl, &muts);  // template with mutation applied
            const string read = Mutate(app, nmut, &gen);  // mutate the read further away from tpl

            try {
                auto ai1 = makeIntegrator(tpl);
                addRead(ai1,
                        MappedRead(Read("N/A", read, mdl), StrandEnum::FORWARD, 0, tpl.length()));
                auto ai2 = makeIntegrator(app);
                addRead(ai2,
                        MappedRead(Read("N/A", read, mdl), StrandEnum::FORWARD, 0, app.length()));
                const double exp  = ai2.LL();
                const double obs0 = ai1.LL();
                const double obs1 = ai1.LL(mut);
                EXPECT_EQ(string(ai1), tpl);
                ai1.ApplyMutations(&muts);
                const double obs2 = ai1.LL();
                // if we're mutating the
                if (nmut == 0) EXPECT_LT(obs0, exp);
                // EXPECT_NEAR(obs1, exp, prec);
                // EXPECT_NEAR(obs2, exp, prec);
                EXPECT_EQ(string(ai1), app);
                EXPECT_EQ(string(ai2), app);
                const double diff1 = std::abs(obs1 - exp);
                const double diff2 = std::abs(obs2 - exp);
                if (diff1 >= prec || diff2 >= prec) {
                    std::cerr << std::endl
                              << "!! intolerable difference: exp: " << exp << ", obs1: " << obs1
                              << ", obs2: " << obs2 << std::endl;
                    std::cerr << "  " << mut << std::endl;
                    std::cerr << "  " << tpl.length() << ", " << tpl << std::endl;
                    std::cerr << "  " << app.length() << ", " << app << std::endl;
                    std::cerr << "  " << read.length() << ", " << read << std::endl;
                    ++nerror;
                }
            } catch (const std::exception& e) {
                std::cerr << std::endl
                          << "!! caught unexpected exception: " << e.what() << std::endl;
                std::cerr << "  " << mut << std::endl;
                std::cerr << "  " << tpl.length() << ", " << tpl << std::endl;
                std::cerr << "  " << app.length() << ", " << app << std::endl;
                std::cerr << "  " << read.length() << ", " << read << std::endl;
                ++nerror;
            }
            ++ntests;
        }
    }

    EXPECT_EQ(nerror, 0);
    // EXPECT_LT(nerror, ntests / 1000);
}

TEST(EvaluatorTest, TestMonoMutationEquivalence)
{
    auto makeMono = [](const string& tpl) { return MonoMolecularIntegrator(tpl, cfg, snr, mdl); };
    auto monoRead = [](MonoMolecularIntegrator& ai, const MappedRead& mr) {
        return ai.AddRead(mr);
    };
    MutationEquivalence(333, 2, makeMono, monoRead);
    MutationEquivalence(333, 1, makeMono, monoRead);
    MutationEquivalence(334, 0, makeMono, monoRead);
}

TEST(EvaluatorTest, TestMultiMutationEquivalence)
{
    auto makeMulti = [](const string& tpl) { return MultiMolecularIntegrator(tpl, cfg); };
    auto multiRead = [](MultiMolecularIntegrator& ai, const MappedRead& mr) {
        return ai.AddRead(mr, snr);
    };
    MutationEquivalence(333, 2, makeMulti, multiRead);
    MutationEquivalence(333, 1, makeMulti, multiRead);
    MutationEquivalence(334, 0, makeMulti, multiRead);
}

// TODO(lhepler): test multi/mono equivalence
// TODO(lhepler): test multiple mutation testing mono and multi

TEST(EvaluatorTest, TestP6C4NoCovAgainstCSharpModel)
{
    const string tpl = "ACGTCGT";
    MultiMolecularIntegrator ai(tpl, cfg);
    ai.AddRead(MappedRead(Read("N/A", "ACGTACGT", mdl), StrandEnum::FORWARD, 0, tpl.length()), snr);
    auto score = [&ai](Mutation&& mut) { return ai.LL(mut) - ai.LL(); };
    EXPECT_NEAR(-4.74517984808494, ai.LL(), prec);
    EXPECT_NEAR(4.00250386364592, score(Mutation(MutationType::INSERTION, 4, 'A')), prec);
    EXPECT_NEAR(-5.19526526492876, score(Mutation(MutationType::SUBSTITUTION, 2, 'C')), prec);
    EXPECT_NEAR(-4.33430539094949, score(Mutation(MutationType::DELETION, 4)), prec);
    EXPECT_NEAR(-9.70299447206563, score(Mutation(MutationType::DELETION, 6)), prec);
    EXPECT_NEAR(-10.5597017942167, score(Mutation(MutationType::DELETION, 0)), prec);
    EXPECT_NEAR(-0.166992912601578, score(Mutation(MutationType::SUBSTITUTION, 4, 'A')), prec);
    EXPECT_NEAR(-1.60697112438296, score(Mutation(MutationType::INSERTION, 4, 'G')), prec);
}