// Author: David Alexander

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/assign/std/vector.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <ConsensusCore/Align/AlignConfig.hpp>
#include <ConsensusCore/Mutation.hpp>
#include <ConsensusCore/Poa/PoaConsensus.hpp>
#include <ConsensusCore/Utils.hpp>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using boost::erase_all_copy;

using namespace boost::assign;  // NOLINT
using namespace ConsensusCore;  // NOLINT

#define MAKE_ALL_PLOTS false

static void plotConsensus(const PoaConsensus* pc, string description,
                          bool REALLY_MAKE_THIS_ONE = false)
{
    if (MAKE_ALL_PLOTS || REALLY_MAKE_THIS_ONE) {
        string dotFname = description + ".dot";
        string pngFname = description + ".png";
        string cmd = string("dot -Tpng ") + dotFname + " > " + pngFname;
        pc->Graph.WriteGraphVizFile(description + ".dot",
                                    (PoaGraph::COLOR_NODES | PoaGraph::VERBOSE_NODES), pc);
        cout << cmd << endl;
        system(cmd.c_str());
    }
}

// TEST(PoaGraph, NoReadsTest)
// {
//  // Test that it works with no reads
//  vector<const SequenceFeatures*> reads;
//  const PoaConsensus* pc = PoaConsensus::findConsensus(reads,
//  PoaConfig::GLOBAL_ALIGNMENT);
//  string dot = pc->getGraph()->toGraphViz();
//  cout << dot << endl;
// }

TEST(PoaGraph, SmallBasicTest)
{
    // Test that it works with a single sequence
    vector<std::string> reads;
    reads += "GGG";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
    string dot = pc->Graph.ToGraphViz();
    string expectedDot =
        "digraph G {"
        "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
        "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
        "2[shape=Mrecord, label=\"{ G | 1 }\"];"
        "3[shape=Mrecord, label=\"{ G | 1 }\"];"
        "4[shape=Mrecord, label=\"{ G | 1 }\"];"
        "0->2 ;"
        "2->3 ;"
        "3->4 ;"
        "4->1 ;"
        "}";
    plotConsensus(pc, "small-basic");
    EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
    EXPECT_EQ("GGG", pc->Sequence);
    delete pc;
}

TEST(PoaGraph, SmallExtraTests)
{
    // Extra at beginning
    {
        vector<std::string> reads;
        reads += "GGG", "TGGG";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ G | 2 }\"];"
            "4[shape=Mrecord, label=\"{ G | 2 }\"];"
            "5[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->2 ;"
            "0->5 ;"
            "}";
        plotConsensus(pc, "extra-at-beginning");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GGG", pc->Sequence);
        delete pc;
    }

    // Extra in middle
    {
        vector<std::string> reads;
        reads += "GGG", "GTGG";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        // this would be easier if we could use the C++0x raw
        // strings feature (in g++ 4.5+)
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ G | 2 }\"];"
            "4[shape=Mrecord, label=\"{ G | 2 }\"];"
            "5[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->3 ;"
            "2->5 ;"
            "}";
        plotConsensus(pc, "extra-in-middle");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GGG", pc->Sequence);
        delete pc;
    }

    // Extra at end
    {
        vector<std::string> reads;
        reads += "GGG", "GGGT";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ G | 2 }\"];"
            "4[shape=Mrecord, label=\"{ G | 2 }\"];"
            "5[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->1 ;"
            "4->5 ;"
            "}";
        plotConsensus(pc, "extra-at-end");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GGG", pc->Sequence);
        delete pc;
    }
}

TEST(PoaGraph, SmallMismatchTests)
{
    // Mismatch at beginning
    {
        vector<std::string> reads;
        reads += "GGG", "TGG";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 1 }\"];"
            "3[shape=Mrecord, label=\"{ G | 2 }\"];"
            "4[shape=Mrecord, label=\"{ G | 2 }\"];"
            "5[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->3 ;"
            "0->5 ;"
            "}";
        plotConsensus(pc, "mismatch-at-beginning");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GG", pc->Sequence);
        delete pc;
    }

    // Mismatch in middle
    {
        vector<std::string> reads;
        reads += "GGG", "GTG", "GTG";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 3 }\"];"
            "3[shape=Mrecord, label=\"{ G | 1 }\"];"
            "4[shape=Mrecord, label=\"{ G | 3 }\"];"
            "5[shape=Mrecord, label=\"{ T | 2 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->4 ;"
            "2->5 ;"
            "}";
        plotConsensus(pc, "mismatch-in-middle");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GTG", pc->Sequence);
        delete pc;
    }

    // Mismatch at end
    {
        vector<std::string> reads;
        reads += "GGG", "GGT";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ G | 2 }\"];"
            "4[shape=Mrecord, label=\"{ G | 1 }\"];"
            "5[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "5->1 ;"
            "3->5 ;"
            "}";
        plotConsensus(pc, "mismatch-at-end");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GG", pc->Sequence);
        delete pc;
    }
}

TEST(PoaGraph, SmallDeletionTests)
{
    // Deletion at beginning
    {
        vector<std::string> reads;
        reads += "GAT", "AT";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 1 }\"];"
            "3[shape=Mrecord, label=\"{ A | 2 }\"];"
            "4[shape=Mrecord, label=\"{ T | 2 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "0->3 ;"
            "}";
        plotConsensus(pc, "deletion-at-beginning");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("AT", pc->Sequence);
        delete pc;
    }

    // Deletion in middle
    {
        vector<std::string> reads;
        reads += "GAT", "GT";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ A | 1 }\"];"
            "4[shape=Mrecord, label=\"{ T | 2 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "2->4 ;"
            "}";
        plotConsensus(pc, "deletion-in-middle");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        delete pc;
    }

    // Deletion at end
    {
        vector<std::string> reads;
        reads += "GAT", "GA";
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
        string dot = pc->Graph.ToGraphViz();
        string expectedDot =
            "digraph G {"
            "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
            "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
            "2[shape=Mrecord, label=\"{ G | 2 }\"];"
            "3[shape=Mrecord, label=\"{ A | 2 }\"];"
            "4[shape=Mrecord, label=\"{ T | 1 }\"];"
            "0->2 ;"
            "2->3 ;"
            "3->4 ;"
            "4->1 ;"
            "3->1 ;"
            "}";
        plotConsensus(pc, "deletion-at-end");
        EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
        EXPECT_EQ("GA", pc->Sequence);
        delete pc;
    }
}

TEST(PoaConsensus, TestSimple)
{
    vector<std::string> reads;
    reads += "TTTACAGGATAGTCCAGT", "ACAGGATACCCCGTCCAGT", "ACAGGATAGTCCAGT",
        "TTTACAGGATAGTCCAGTCCCC", "TTTACAGGATTAGTCCAGT", "TTTACAGGATTAGGTCCCAGT",
        "TTTACAGGATAGTCCAGT";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
    plotConsensus(pc, "simple");
    EXPECT_EQ("TTTACAGGATAGTCCAGT", pc->Sequence);
    delete pc;
}

TEST(PoaConsensus, TestOverhangSecond)
{
    vector<std::string> reads;
    reads += "TTTACAGGATAGTCCAGT", "TTTACAGGATAGTCCAGTAAA", "TTTACAGGATAGTCCAGTAAA";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
    EXPECT_EQ("TTTACAGGATAGTCCAGTAAA", pc->Sequence);
    delete pc;
}

TEST(PoaConsensus, SmallSemiglobalTest)
{
    vector<std::string> reads;
    reads += "GGTGG", "GGTGG", "T";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, SEMIGLOBAL);
    plotConsensus(pc, "small-semiglobal");
    string expectedDot =
        "digraph G {"
        "0[shape=Mrecord, label=\"{ ^ | 0 }\"];"
        "1[shape=Mrecord, label=\"{ $ | 0 }\"];"
        "2[shape=Mrecord, label=\"{ G | 2 }\"];"
        "3[shape=Mrecord, label=\"{ G | 2 }\"];"
        "4[shape=Mrecord, label=\"{ T | 3 }\"];"
        "5[shape=Mrecord, label=\"{ G | 2 }\"];"
        "6[shape=Mrecord, label=\"{ G | 2 }\"];"
        "0->2 ;"
        "2->3 ;"
        "3->4 ;"
        "4->5 ;"
        "5->6 ;"
        "6->1 ;"
        "4->1 ;"
        "0->4 ;"
        "}";
    string dot = pc->Graph.ToGraphViz();
    EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
    EXPECT_EQ("GGTGG", pc->Sequence);
    delete pc;
}

TEST(PoaConsensus, SmallTilingTest)
{
    vector<std::string> reads;
    reads += "GGGGAAAA", "AAAATTTT", "TTTTCCCC", "CCCCAGGA";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, SEMIGLOBAL);
    plotConsensus(pc, "small-tiling");
    EXPECT_EQ("GGGGAAAATTTTCCCCAGGA", pc->Sequence);
    delete pc;
}

TEST(PoaConsensus, TestVerboseGraphVizOutput)
{
    vector<std::string> reads;
    reads += "GGG", "TGGG";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
    string dot = pc->Graph.ToGraphViz(PoaGraph::COLOR_NODES | PoaGraph::VERBOSE_NODES, pc);

    string expectedDot =
        "digraph G {"
        "0[shape=Mrecord, label=\"{ { 0 | ^ } |{ 0 | 1 } |{ 0.00 | 0.00 } }\"];"
        "1[shape=Mrecord, label=\"{ { 1 | $ } |{ 0 | 0 } |{ 0.00 | 0.00 } }\"];"
        "2[shape=Mrecord, style=\"filled\", fillcolor=\"lightblue\" ,"
        " label=\"{ { 2 | G } |{ 2 | 2 } |{ 2.00 | 2.00 } }\"];"
        "3[shape=Mrecord, style=\"filled\", fillcolor=\"lightblue\" ,"
        " label=\"{ { 3 | G } |{ 2 | 2 } |{ 2.00 | 4.00 } }\"];"
        "4[shape=Mrecord, style=\"filled\", fillcolor=\"lightblue\" ,"
        " label=\"{ { 4 | G } |{ 2 | 0 } |{ 2.00 | 6.00 } }\"];"
        "5[shape=Mrecord, label=\"{ { 5 | T } |{ 1 | 0 } |{ -0.00 | -0.00 } }\"];"
        "0->2 ;"
        "2->3 ;"
        "3->4 ;"
        "4->1 ;"
        "5->2 ;"
        "0->5 ;}";

    EXPECT_EQ(expectedDot, erase_all_copy(dot, "\n"));
    delete pc;
}

TEST(PoaConsensus, TestLocalStaggered)
{
    // Adapted from Pat's C# test
    vector<std::string> reads;
    reads += "TTTACAGGATAGTGCCGCCAATCTTCCAGT", "GATACCCCGTGCCGCCAATCTTCCAGTATATACAGCACGAGTAGC",
        "ATAGTGCCGCCAATCTTCCAGTATATACAGCACGGAGTAGCATCACGTACGTACGTCTACACGTAATT",
        "ACGTCTACACGTAATTTTGGAGAGCCCTCTCTCACG", "ACACGTAATTTTGGAGAGCCCTCTCTTCACG",
        "AGGATAGTGCCGCCAATCTTCCAGTAATATACAGCACGGAGTAGCATCACGTACG",
        "ATAGTGCCGCCAATCTTCCAGTATATACAGCACGGAGTAGCATCACGTACGTACGTCTACACGT";

    // 4 is a magic number here.  the .NET code had an entrenched
    // assumption that sequences in POA were subreads from a ZMW, so
    // the minCoverage was (numReads - 3), under assumption that basal
    // coverage for CCS is (numReads-2) (beginning, end read).
    // Application has to provide a sensible minCoverage.
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, LOCAL, 4);
    plotConsensus(pc, "local-staggered", true);
    EXPECT_EQ("ATAGTGCCGCCAATCTTCCAGTATATACAGCACGGAGTAGCATCACGTACGTACGTCTACACGTAATT", pc->Sequence);
    delete pc;
}

TEST(PoaConsensus, TestLongInsert)
{
    // Adapted from Pat's C# test
    vector<std::string> reads;
    reads +=
        "TTTACAGGATAGTGCCGCCAATCTTCCAGTGATACCCCGTGCCGCCAATCTTCCAGTATATACAGCA"
        "CGAGGTAGC",
        "TTTACAGGATAGTGCCGGCCAATCTTCCAGTGATACCCCGTGCCGCCAATCTTCCAGTATATACAGCACGAG"
        "TAGC",
        "TTGTACAGGATAGTGCCGCCAATCTTCCAGTGATGGGGGGGGGGGGGGGGGGGGGGGGGGGACCCCGTGCCG"
        "CCAATCTTCCAGTATATACAGCACGAGTAGC";
    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
    EXPECT_EQ(
        "TTTACAGGATAGTGCCGCCAATCTTCCAGTGATACCCCGTGCCGCCAATCTTCCAGTATATACAGC"
        "ACGAGTAGC",
        pc->Sequence);
    delete pc;
}

#if 0
TEST(PoaConsensus, TestMutations)
{
    using ::testing::ElementsAreArray;

    vector<std::string> reads;
    reads += "TGATTACAT",
             "TGATTACAT",
             "TGATTCAT",    // Deletion @ 5
             "TGATTATAT",   // Substitution @ 6
             "TGATTGACAT";  // Insertion @ 5

    const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);

    const std::vector<ScoredMutation>* scoredMutations = pc->Mutations();
    std::vector<string> variantDescriptions;
    foreach (const ScoredMutation& scoredMutation, *scoredMutations)
    {
        variantDescriptions.push_back(scoredMutation.ToString());
    }
    sort(variantDescriptions.begin(), variantDescriptions.end());
    const char* expectedDescriptions[] = { "Deletion @5:6 -3.00",
                                           "Insertion (G) @5 -3.00",
                                           "Substitution (T) @6:7 -3.00" };
    ASSERT_THAT(variantDescriptions, ElementsAreArray(expectedDescriptions));
    delete pc;
}
#endif

TEST(PoaConsensus, NondeterminismRegressionTest)
{
    //
    // This is a regression test for a real-world case of
    // nondeterminism found in the POA on a quiver job on Staph.
    //
    std::vector<std::string> reads;
    reads +=
        "TATCAATCAACGAAATTCGCCAATTCCGTCATGAATGTCAATATCTAACTACACTTTAGAATACATTCTT"
        "TGACATGCCTGGCCTATTGATATTTCAATAAAATCAGACTATAAAGACAACTTACAAATGATCCTATAAA"
        "TTAAAGATCGAGAATCTAAAGAGTGAAATTAAAGCTAATTACTGCTTTAAAAATTTTACGTGCACACAAA"
        "AATGAATTTATCCTCATTATATCGAAAATACCATGAAGTATAGTAAGCTAACTTGAATATGATCATTAAT"
        "CGGCTATATGATTATTTTGATAATGCAATGAGCATCAATCTGAATTTATGACCTATCATTCGCGTTGCAT"
        "TTATTGAAGTGAAAATTCATGTACGCTTTTTTATTTTATTAATATAATCCTTGATATTGGTTATATACCA"
        "CGCTGTCACATAATTTTCAATAAATTTTTCTACTAAATGAAGTGTCTGTTATCTATCAC";
    reads +=
        "TATCAACAACGAAAATGCGCAGTTACGTCATGATTTATGTCAAATAATCTAAACGACACTTTCAGAAATA"
        "AATACATTCGAGAAGATGAATGCCTGGCGCAAAGTGATTATTTCAATAAAATATTTGTACCTTGAAAGAC"
        "AATTTACAAATGAATGCTATAAAATTTAAATGGATCCGGAGAATCTTTAAAGTACGTGAAATTAAAGGCT"
        "AAGATTACTGCGAAAAATTTTCGTGCACAAGAAATGAATGTTCCAGATTAGTATCGGAAAATAAGCCATG"
        "AAGAAGCTAGCATTAACTTGAATATGATCGATTTAATCGGCAGTATTGGTAATTATCTTGATAAGCAATT"
        "GAGCATCAACTGAAATTGAATGACTCTACATGCCTCGCTGAGTATGCGATTTATTGAAAGTGAAATTCAG"
        "TAAAGTTTATTGTTATGAATAAATGCGTACTTGGATGAATATCCCGACGGTAGTTCAAGTGTAAATGGAG"
        "TGAGGGGGTTCTTTCTTATAGAATAGTTTTATACTACTGATAAGGTGTAACCTGAGTGAGTCGTGATTTT"
        "AGAGTTACTTGCGAAC";

    std::set<std::string> answers;
    for (int run = 0; run < 100; run++) {
        const PoaConsensus* pc = PoaConsensus::FindConsensus(reads, GLOBAL);
#if 0
        char fname[100];
        sprintf(fname, "/tmp/gr%03d.dot", run);
        pc->WriteGraphVizFile(fname, (PoaGraph::VERBOSE_NODES | PoaGraph::COLOR_NODES));
#endif
        answers.insert(pc->Sequence);
        delete pc;
    }
    ASSERT_EQ(1, answers.size());
}
