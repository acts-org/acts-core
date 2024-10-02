// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This ROOT script compares two ROOT files in an order-insensitive way. Its
// intended use is to compare the output of a single-threaded and multi-threaded
// programs in order to check that results are perfectly reproducible.
//
// As a current limitation, which may be lifted in the future, the script does
// all of its processing in RAM, which means that the input dataset must fit in
// RAM. So do not try to run this on terabytes of data. You don't need that much
// data to check that your multithreaded program runs well anyhow.
//
// Another limitation is that the comparison relies on perfect output
// reproducibility, which is a very costly guarantee to achieve in a
// multi-threaded environment. If you want to compare "slightly different"
// outputs, this script will not work as currently written. I cannot think of a
// way in which imperfect reproducibility could be checked in a manner which
// doesn't depend on the details of the data being compared.

#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "TBranch.h"
#include "TFile.h"
#include "TKey.h"
#include "TList.h"
#include "TObject.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "compareRootFiles.hpp"

// Minimal mechanism for assertion checking and comparison
#define ASSERT(pred, msg)          \
  if (!(pred)) {                   \
    std::cout << msg << std::endl; \
    return 1;                      \
  }

#define ASSERT_EQUAL(v1, v2, msg) \
  ASSERT((v1) == (v2), msg << "(" << (v1) << " vs " << (v2) << ") ")

#define ASSERT_STR_EQUAL(s1, s2, msg) \
  ASSERT(strcmp((s1), (s2)) == 0, msg << " (" << (s1) << " vs " << (s2) << ") ")

// This script returns 0 if the files have identical contents except for event
// ordering, and a nonzero result if the contents differ or an error occurred.
//
// If the optional dump_data_on_failure flag is set, it will also dump the
// mismatching event data to stdout on failure for manual inspection.
//
// If the optional skip_unsupported_branches flag is set, the script will ignore
// unsupported branch types in the input file instead of aborting.
//

int compareRootFiles(std::string file1, std::string file2,
                     bool dump_data_on_failure = false,
                     bool skip_unsupported_branches = false) {
  std::cout << "Comparing ROOT files " << file1 << " and " << file2
            << std::endl;

  std::cout << "* Opening the files..." << std::endl;
  HomogeneousPair<TFile> files{file1.c_str(), file2.c_str()};
  if (files.first.IsZombie()) {
    std::cout << "  - Could not open file " << file1 << "!" << std::endl;
    return 2;
  } else if (files.second.IsZombie()) {
    std::cout << "  - Could not open file " << file2 << "!" << std::endl;
    return 2;
  }

  std::cout << "* Extracting file keys..." << std::endl;
  HomogeneousPair<std::vector<TKey*>> fileKeys;
  {
    // This is how we would extract keys from one file
    const auto loadKeys = [](const TFile& file, std::vector<TKey*>& target) {
      const int keyCount = file.GetNkeys();
      target.reserve(keyCount);
      TIter keyIter{file.GetListOfKeys()};
      for (int i = 0; i < keyCount; ++i) {
        target.emplace_back(dynamic_cast<TKey*>(keyIter()));
      }
    };

    // Do it for each of our files
    loadKeys(files.first, fileKeys.first);
    loadKeys(files.second, fileKeys.second);
  }

  std::cout << "* Selecting the latest key cycle..." << std::endl;
  std::vector<HomogeneousPair<TKey*>> keyPairs;
  {
    // For each file and for each key name, we want to know what is the latest
    // key cycle, and who is the associated key object
    using KeyMetadata = std::pair<short, TKey*>;
    using FileMetadata = std::map<std::string, KeyMetadata>;
    HomogeneousPair<FileMetadata> metadata;

    // This is how we compute this metadata for a single file
    const auto findLatestCycle = [](const std::vector<TKey*>& keys,
                                    FileMetadata& target) {
      // Iterate through the file's keys
      for (const auto key : keys) {
        // Extract information about the active key
        const std::string keyName{key->GetName()};
        const short newCycle{key->GetCycle()};

        // Do we already know of a key with the same name?
        auto latestCycleIter = target.find(keyName);
        if (latestCycleIter != target.end()) {
          // If so, keep the key with the most recent cycle number
          auto& latestCycleMetadata = latestCycleIter->second;
          if (newCycle > latestCycleMetadata.first) {
            latestCycleMetadata = {newCycle, key};
          }
        } else {
          // If not, this is obviously the most recent key we've seen so
          // far
          target.emplace(keyName, KeyMetadata{newCycle, key});
        }
      }
    };

    // We'll compute this information for both of our files...
    std::cout << "  - Finding the latest cycle for each file..." << std::endl;
    findLatestCycle(fileKeys.first, metadata.first);
    findLatestCycle(fileKeys.second, metadata.second);

    // ...and then we'll group the latest keys by name, detect keys which only
    // exist in a single file along the way, and report that as an error
    std::cout << "  - Grouping per-file latest keys..." << std::endl;
    {
      // Make sure that both files have the same amount of keys once duplicate
      // versions are removed
      const auto f1KeyCount = metadata.first.size();
      const auto f2KeyCount = metadata.second.size();
      ASSERT_EQUAL(f1KeyCount, f2KeyCount,
                   "    o Number of keys does not match");
      keyPairs.reserve(f1KeyCount);

      // Iterate through the keys, in the same order (as guaranteed by std::map)
      for (auto f1MetadataIter = metadata.first.cbegin(),
                f2MetadataIter = metadata.second.cbegin();
           f1MetadataIter != metadata.first.cend();
           ++f1MetadataIter, ++f2MetadataIter) {
        // Do the keys have the same name?
        const auto& f1KeyName = f1MetadataIter->first;
        const auto& f2KeyName = f2MetadataIter->first;
        ASSERT_EQUAL(f1KeyName, f2KeyName, "    o Key names do not match");

        // If so, extract the associated key pair
        keyPairs.emplace_back(f1MetadataIter->second.second,
                              f2MetadataIter->second.second);
      }
    }
  }

  std::cout << "* Comparing key metadata..." << std::endl;
  for (const auto& keyPair : keyPairs) {
    const auto& key1 = keyPair.first;
    const auto& key2 = keyPair.second;

    ASSERT_STR_EQUAL(key1->GetClassName(), key2->GetClassName(),
                     "  - Class name does not match!");
    ASSERT_STR_EQUAL(key1->GetTitle(), key2->GetTitle(),
                     "  - Title does not match!");
    ASSERT_EQUAL(key1->GetVersion(), key2->GetVersion(),
                 "  - Key version does not match!");
  }

  // NOTE: The current version of this script only supports some file contents.
  //       It may be extended later if the need for other data formats arise.
  std::cout << "* Extracting TTrees..." << std::endl;
  std::vector<HomogeneousPair<TTree*>> treePairs;
  std::vector<HomogeneousPair<TVectorT<float>*>> vectorPairs;
  std::vector<HomogeneousPair<TEfficiency*>> efficiencyPairs;
  std::vector<HomogeneousPair<TProfile*>> profilePairs;
  std::vector<HomogeneousPair<TH2F*>> th2fPairs;

  for (const auto& keyPair : keyPairs) {
    TObject* obj1 = keyPair.first->ReadObj();
    TObject* obj2 = keyPair.second->ReadObj();

    ASSERT_STR_EQUAL(obj1->ClassName(), obj2->ClassName(),
                     "  - Object type does not match!");

    // Check if the object is a TTree
    bool isTTree = strcmp(obj1->ClassName(), "TTree") == 0;

    if (isTTree) {
      TTree* tree1 = dynamic_cast<TTree*>(obj1);
      TTree* tree2 = dynamic_cast<TTree*>(obj2);
      if (tree1 && tree2) {
        treePairs.emplace_back(tree1, tree2);
      }
      continue;  // Skip the rest of the loop
    }

    bool isTVector = strcmp(obj1->ClassName(), "TVectorT<float>") == 0;

    if (isTVector) {
      TVectorT<float>* vector1 = dynamic_cast<TVectorT<float>*>(obj1);
      TVectorT<float>* vector2 = dynamic_cast<TVectorT<float>*>(obj2);
      if (vector1 && vector2) {
        vectorPairs.emplace_back(vector1, vector2);
      }
      continue;  // Skip the rest of the loop
    }

    bool isTEfficiency = strcmp(obj1->ClassName(), "TEfficiency") == 0;

    if (isTEfficiency) {
      TEfficiency* efficiency1 = dynamic_cast<TEfficiency*>(obj1);
      TEfficiency* efficiency2 = dynamic_cast<TEfficiency*>(obj2);
      if (efficiency1 && efficiency2) {
        efficiencyPairs.emplace_back(efficiency1, efficiency2);
      }
      continue;  // Skip the rest of the loop
    }

    bool isTProfile = strcmp(obj1->ClassName(), "TProfile") == 0;

    if (isTProfile) {
      TProfile* profile1 = dynamic_cast<TProfile*>(obj1);
      TProfile* profile2 = dynamic_cast<TProfile*>(obj2);
      if (profile1 && profile2) {
        profilePairs.emplace_back(profile1, profile2);
      }
      continue;  // Skip the rest of the loop
    }

    bool isTH2F = strcmp(obj1->ClassName(), "TH2F") == 0;

    if (isTH2F) {
      TH2F* th2f1 = dynamic_cast<TH2F*>(obj1);
      TH2F* th2f2 = dynamic_cast<TH2F*>(obj2);
      if (th2f1 && th2f2) {
        th2fPairs.emplace_back(th2f1, th2f2);
      }
      continue;  // Skip the rest of the loop
    }

    ASSERT(false, "  - Input " << obj1->ClassName() << " is not supported!");
  }

  std::cout << "* Comparing the trees..." << std::endl;
  for (const auto& treePair : treePairs) {
    const auto& tree1 = treePair.first;
    const auto& tree2 = treePair.second;

    std::cout << "  - Comparing tree " << tree1->GetName() << "..."
              << std::endl;

    std::cout << "    o Comparing tree-wide metadata..." << std::endl;
    const std::size_t t1EntryCount = tree1->GetEntries();
    {
      const std::size_t t2EntryCount = tree2->GetEntries();
      ASSERT_EQUAL(t1EntryCount, t2EntryCount,
                   "      ~ Number of entries does not match!");
    }

    if (t1EntryCount == 0) {
      std::cout << "    o Skipping empty tree!" << std::endl;
      continue;
    }

    std::cout << "    o Preparing for tree readout..." << std::endl;
    TTreeReader t1Reader(tree1);
    TTreeReader t2Reader(tree2);
    BranchComparisonHarness::TreeMetadata treeMetadata{t1Reader, t2Reader,
                                                       t1EntryCount};

    std::cout << "    o Comparing branch metadata..." << std::endl;
    std::vector<HomogeneousPair<TBranch*>> branchPairs;
    {
      // Check number of branches and allocate branch storage
      const int t1BranchCount = tree1->GetNbranches();
      const int t2BranchCount = tree2->GetNbranches();
      ASSERT_EQUAL(t1BranchCount, t2BranchCount,
                   "      ~ Number of branches does not match!");
      branchPairs.reserve(t1BranchCount);

      // Extract branches using TTree::GetListOfBranches()
      TIter t1BranchIter{tree1->GetListOfBranches()};
      TIter t2BranchIter{tree2->GetListOfBranches()};
      for (int i = 0; i < t1BranchCount; ++i) {
        branchPairs.emplace_back(dynamic_cast<TBranch*>(t1BranchIter()),
                                 dynamic_cast<TBranch*>(t2BranchIter()));
      }
    }

    std::cout << "    o Setting up branch-specific processing..." << std::endl;
    std::vector<BranchComparisonHarness> branchComparisonHarnesses;
    branchComparisonHarnesses.reserve(branchPairs.size());
    for (const auto& branchPair : branchPairs) {
      const auto& branch1 = branchPair.first;
      const auto& branch2 = branchPair.second;

      std::cout << "      ~ Checking branch metadata..." << std::endl;
      std::string b1ClassName, b1BranchName;
      EDataType b1DataType;
      {
        std::string b2ClassName, b2BranchName;
        EDataType b2DataType;
        TClass* unused;

        b1ClassName = branch1->GetClassName();
        b2ClassName = branch2->GetClassName();
        ASSERT_EQUAL(b1ClassName, b2ClassName,
                     "        + Class name does not match!");
        branch1->GetExpectedType(unused, b1DataType);
        branch2->GetExpectedType(unused, b2DataType);
        ASSERT_EQUAL(b1DataType, b2DataType,
                     "        + Raw data type does not match!");
        const int b1LeafCount = branch1->GetNleaves();
        const int b2LeafCount = branch2->GetNleaves();
        ASSERT_EQUAL(b1LeafCount, b2LeafCount,
                     "        + Number of leaves does not match!");
        ASSERT_EQUAL(
            b1LeafCount, 1,
            "        + Branches with several leaves are not supported!");
        b1BranchName = branch1->GetName();
        b2BranchName = branch2->GetName();
        ASSERT_EQUAL(b1BranchName, b2BranchName,
                     "        + Branch name does not match!");
      }

      std::cout << "      ~ Building comparison harness for branch "
                << b1BranchName << "..." << std::endl;
      try {
        auto branchHarness = BranchComparisonHarness::create(
            treeMetadata, b1BranchName, b1DataType, b1ClassName);
        branchComparisonHarnesses.emplace_back(std::move(branchHarness));
      } catch (BranchComparisonHarness::UnsupportedBranchType) {
        // When encountering an unsupported branch type, we can either skip
        // the branch or abort depending on configuration
        std::cout << "        + Unsupported branch type! "
                  << "(eDataType: " << b1DataType << ", ClassName: \""
                  << b1ClassName << "\")" << std::endl;
        if (skip_unsupported_branches) {
          continue;
        } else {
          return 3;
        }
      }
    }

    std::cout << "    o Reading event data..." << std::endl;
    for (std::size_t i = 0; i < t1EntryCount; ++i) {
      // Move to the next TTree entry (= next event)
      t1Reader.Next();
      t2Reader.Next();

      // Load the data associated with each branch
      for (auto& branchHarness : branchComparisonHarnesses) {
        branchHarness.loadCurrentEvent();
      }
    }

    std::cout << "    o Sorting the first tree..." << std::endl;
    {
      std::cout << "      ~ Defining event comparison operator..." << std::endl;
      IndexComparator t1CompareEvents = [&branchComparisonHarnesses](
                                            std::size_t i,
                                            std::size_t j) -> Ordering {
        for (auto& branchHarness : branchComparisonHarnesses) {
          const auto order = branchHarness.sortHarness.first.first(i, j);
          if (order != Ordering::EQUAL) {
            return order;
          }
        }
        return Ordering::EQUAL;
      };

      std::cout << "      ~ Defining event swapping operator..." << std::endl;
      IndexSwapper t1SwapEvents = [&branchComparisonHarnesses](std::size_t i,
                                                               std::size_t j) {
        for (auto& branchHarness : branchComparisonHarnesses) {
          branchHarness.sortHarness.first.second(i, j);
        }
      };

      std::cout << "      ~ Running quicksort on the tree..." << std::endl;
      quickSort(0, t1EntryCount - 1, t1CompareEvents, t1SwapEvents);
    }

    std::cout << "    o Sorting the second tree..." << std::endl;
    {
      std::cout << "      ~ Defining event comparison operator..." << std::endl;
      IndexComparator t2CompareEvents = [&branchComparisonHarnesses](
                                            std::size_t i,
                                            std::size_t j) -> Ordering {
        for (auto& branchHarness : branchComparisonHarnesses) {
          const auto order = branchHarness.sortHarness.second.first(i, j);
          if (order != Ordering::EQUAL) {
            return order;
          }
        }
        return Ordering::EQUAL;
      };

      std::cout << "      ~ Defining event swapping operator..." << std::endl;
      IndexSwapper t2SwapEvents = [&branchComparisonHarnesses](std::size_t i,
                                                               std::size_t j) {
        for (auto& branchHarness : branchComparisonHarnesses) {
          branchHarness.sortHarness.second.second(i, j);
        }
      };

      std::cout << "      ~ Running quicksort on the tree..." << std::endl;
      quickSort(0, t1EntryCount - 1, t2CompareEvents, t2SwapEvents);
    }

    std::cout << "    o Checking that both trees are now equal..." << std::endl;
    for (auto& branchHarness : branchComparisonHarnesses) {
      std::cout << "      ~ Comparing branch " << branchHarness.branchName
                << "..." << std::endl;
      if (!branchHarness.eventDataEqual()) {
        std::cout << "        + Branch contents do not match!" << std::endl;
        if (dump_data_on_failure) {
          std::cout << "        + Dumping branch contents:" << std::endl;
          branchHarness.dumpEventData();
        }
        return 4;
      }
    }
  }

  std::cout << "* Comparing the vectors..." << std::endl;
  for (const auto& vectorPair : vectorPairs) {
    const auto& vector1 = vectorPair.first;
    const auto& vector2 = vectorPair.second;

    std::cout << "  - Comparing vector " << vector1->GetName() << "..."
              << std::endl;

    std::cout << "    o Comparing vector-wide metadata..." << std::endl;
    const std::size_t v1Size = vector1->GetNoElements();
    {
      const std::size_t v2Size = vector2->GetNoElements();
      ASSERT_EQUAL(v1Size, v2Size,
                   "      ~ Number of elements does not match!");
    }

    if (v1Size == 0) {
      std::cout << "    o Skipping empty vector!" << std::endl;
      continue;
    }

    std::cout << "    o Comparing vector data..." << std::endl;
    for (std::size_t i = 0; i < v1Size; ++i) {
      ASSERT_EQUAL(vector1->operator[](i), vector2->operator[](i),
                   "      ~ Vector elements do not match!");
    }
  }

  std::cout << "* Comparing the efficiencies..." << std::endl;
  for (const auto& efficiencyPair : efficiencyPairs) {
    const auto& efficiency1 = efficiencyPair.first;
    const auto& efficiency2 = efficiencyPair.second;

    std::cout << "  - Comparing efficiency " << efficiency1->GetName() << "..."
              << std::endl;

    std::cout << "    o Comparing efficiency-wide metadata..." << std::endl;
    const std::size_t e1Size = efficiency1->GetTotalHistogram()->GetEntries();
    {
      const std::size_t e2Size = efficiency2->GetTotalHistogram()->GetEntries();
      ASSERT_EQUAL(e1Size, e2Size, "      ~ Number of entries does not match!");
    }

    if (e1Size == 0) {
      std::cout << "    o Skipping empty efficiency!" << std::endl;
      continue;
    }

    std::cout << "    o Comparing efficiency data..." << std::endl;
    for (std::size_t i = 0; i < e1Size; ++i) {
      ASSERT_EQUAL(efficiency1->GetEfficiency(i), efficiency2->GetEfficiency(i),
                   "      ~ Efficiency elements do not match!");
    }
  }

  std::cout << "* Comparing the profiles..." << std::endl;
  for (const auto& profilePair : profilePairs) {
    const auto& profile1 = profilePair.first;
    const auto& profile2 = profilePair.second;

    std::cout << "  - Comparing profile " << profile1->GetName() << "..."
              << std::endl;

    std::cout << "    o Comparing profile-wide metadata..." << std::endl;
    const std::size_t p1Size = profile1->GetEntries();
    {
      const std::size_t p2Size = profile2->GetEntries();
      ASSERT_EQUAL(p1Size, p2Size, "      ~ Number of entries does not match!");
    }

    if (p1Size == 0) {
      std::cout << "    o Skipping empty profile!" << std::endl;
      continue;
    }

    std::cout << "    o Comparing profile data..." << std::endl;
    for (std::size_t i = 0; i < p1Size; ++i) {
      ASSERT_EQUAL(profile1->GetBinContent(i), profile2->GetBinContent(i),
                   "      ~ Profile elements do not match!");
    }
  }

  std::cout << "* Comparing the TH2Fs..." << std::endl;
  for (const auto& th2fPair : th2fPairs) {
    const auto& th2f1 = th2fPair.first;
    const auto& th2f2 = th2fPair.second;

    std::cout << "  - Comparing TH2F " << th2f1->GetName() << "..."
              << std::endl;

    std::cout << "    o Comparing TH2F-wide metadata..." << std::endl;
    const std::size_t th2f1Size = th2f1->GetEntries();
    {
      const std::size_t th2f2Size = th2f2->GetEntries();
      ASSERT_EQUAL(th2f1Size, th2f2Size,
                   "      ~ Number of entries does not match!");
    }

    if (th2f1Size == 0) {
      std::cout << "    o Skipping empty TH2F!" << std::endl;
      continue;
    }

    std::cout << "    o Comparing TH2F data..." << std::endl;
    for (std::size_t i = 0; i < th2f1Size; ++i) {
      ASSERT_EQUAL(th2f1->GetBinContent(i), th2f2->GetBinContent(i),
                   "      ~ TH2F elements do not match!");
    }
  }

  std::cout << "* Input files are equal, event order aside!" << std::endl;
  return 0;
}
