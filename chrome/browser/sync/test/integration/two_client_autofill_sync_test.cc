// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/utf_string_conversions.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/autofill_helper.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/autofill/browser/autofill_profile.h"
#include "components/autofill/browser/credit_card.h"
#include "components/autofill/browser/personal_data_manager.h"
#include "components/autofill/browser/webdata/autofill_entry.h"
#include "components/autofill/browser/webdata/autofill_table.h"

using autofill::AutofillKey;
using autofill::AutofillTable;
using autofill::AutofillProfile;
using autofill::AutofillType;
using autofill::CreditCard;
using autofill::PersonalDataManager;
using autofill_helper::AddKeys;
using autofill_helper::AddProfile;
using autofill_helper::CreateAutofillProfile;
using autofill_helper::GetAllKeys;
using autofill_helper::GetAllProfiles;
using autofill_helper::GetPersonalDataManager;
using autofill_helper::KeysMatch;
using autofill_helper::ProfilesMatch;
using autofill_helper::PROFILE_FRASIER;
using autofill_helper::PROFILE_HOMER;
using autofill_helper::PROFILE_MARION;
using autofill_helper::PROFILE_NULL;
using autofill_helper::RemoveKey;
using autofill_helper::RemoveProfile;
using autofill_helper::SetCreditCards;
using autofill_helper::UpdateProfile;
using bookmarks_helper::AddFolder;
using bookmarks_helper::AddURL;
using bookmarks_helper::IndexedURL;
using bookmarks_helper::IndexedURLTitle;

class TwoClientAutofillSyncTest : public SyncTest {
 public:
  TwoClientAutofillSyncTest() : SyncTest(TWO_CLIENT) { count = 0; }
  virtual ~TwoClientAutofillSyncTest() {}

  // We do this so as to make a change that will trigger the autofill to sync.
  // By default autofill does not sync unless there is some other change.
  void MakeABookmarkChange(int profile) {
    ASSERT_TRUE(
        AddURL(profile, IndexedURLTitle(count), GURL(IndexedURL(count))));
    ++count;
  }
 private:
  int count;
  DISALLOW_COPY_AND_ASSIGN(TwoClientAutofillSyncTest);
};

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, WebDataServiceSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a key.
  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0"));
  AddKeys(0, keys);
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(1U, GetAllKeys(0).size());

  // Client1 adds a key.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-0"));
  AddKeys(1, keys);
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(2U, GetAllKeys(0).size());

  // Client0 adds a key with the same name.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-1"));
  AddKeys(0, keys);
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(3U, GetAllKeys(0).size());

  // Client1 removes a key.
  RemoveKey(1, AutofillKey("name1", "value1-0"));
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(2U, GetAllKeys(0).size());

  // Client0 removes the rest.
  RemoveKey(0, AutofillKey("name0", "value0"));
  RemoveKey(0, AutofillKey("name1", "value1-1"));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(0U, GetAllKeys(0).size());
}

// TCM ID - 3678296.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddUnicodeProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey(WideToUTF16(L"Sigur R\u00F3s"),
                          WideToUTF16(L"\u00C1g\u00E6tis byrjun")));
  AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(KeysMatch(0, 1));
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToSameProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0-0"));
  keys.insert(AutofillKey("name0", "value0-1"));
  keys.insert(AutofillKey("name1", "value1"));
  AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(2U, GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToDifferentProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys0;
  keys0.insert(AutofillKey("name0", "value0-0"));
  keys0.insert(AutofillKey("name1", "value1"));
  AddKeys(0, keys0);

  std::set<AutofillKey> keys1;
  keys1.insert(AutofillKey("name0", "value0-1"));
  keys1.insert(AutofillKey("name2", "value2"));
  keys1.insert(AutofillKey("name3", "value3"));
  AddKeys(1, keys1);

  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(KeysMatch(0, 1));
  ASSERT_EQ(5U, GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       PersonalDataManagerSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a profile.
  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  // Client1 adds a profile.
  AddProfile(1, CreateAutofillProfile(PROFILE_MARION));
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());

  // Client0 adds the same profile.
  AddProfile(0, CreateAutofillProfile(PROFILE_MARION));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());

  // Client1 removes a profile.
  RemoveProfile(1, GetAllProfiles(1)[0]->guid());
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  // Client0 updates a profile.
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                ASCIIToUTF16("Bart"));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  // Client1 removes remaining profile.
  RemoveProfile(1, GetAllProfiles(1)[0]->guid());
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(0U, GetAllProfiles(0).size());
}

// TCM ID - 7261786.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddDuplicateProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
}

// TCM ID - 3636294.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, SameProfileWithConflict) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AutofillProfile profile0 = CreateAutofillProfile(PROFILE_HOMER);
  AutofillProfile profile1 = CreateAutofillProfile(PROFILE_HOMER);
  profile1.SetRawInfo(autofill::PHONE_HOME_WHOLE_NUMBER,
                      ASCIIToUTF16("1234567890"));

  AddProfile(0, profile0);
  AddProfile(1, profile1);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
}

// TCM ID - 3626291.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddEmptyProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_NULL));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(0U, GetAllProfiles(0).size());
}

// TCM ID - 3616283.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
}

// TCM ID - 3632260.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddMultipleProfiles) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(0, CreateAutofillProfile(PROFILE_MARION));
  AddProfile(0, CreateAutofillProfile(PROFILE_FRASIER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(3U, GetAllProfiles(0).size());
}

// TCM ID - 3602257.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DeleteProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  RemoveProfile(1, GetAllProfiles(1)[0]->guid());
  MakeABookmarkChange(1);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(0U, GetAllProfiles(0).size());
}

// TCM ID - 3627300.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MergeProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(1, CreateAutofillProfile(PROFILE_MARION));
  AddProfile(1, CreateAutofillProfile(PROFILE_FRASIER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(3U, GetAllProfiles(0).size());
}

// TCM ID - 3665264.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, UpdateFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                ASCIIToUTF16("Lisa"));
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                ASCIIToUTF16("grrrl@TV.com"));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
}

// TCM ID - 3628299.
// This test is flaky on all platforms.  See crbug.com/152551.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DISABLED_ConflictingFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                ASCIIToUTF16("Lisa"));
  MakeABookmarkChange(0);
  UpdateProfile(1,
                GetAllProfiles(1)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                ASCIIToUTF16("Bart"));
  MakeABookmarkChange(1);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());
}

// TCM ID - 3663293.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DisableAutofill) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  ASSERT_TRUE(GetClient(0)->DisableSyncForDatatype(syncer::AUTOFILL));
  AddProfile(0, CreateAutofillProfile(PROFILE_FRASIER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_FALSE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());
  ASSERT_EQ(1U, GetAllProfiles(1).size());

  ASSERT_TRUE(GetClient(0)->EnableSyncForDatatype(syncer::AUTOFILL));
  MakeABookmarkChange(0);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());
}

// TCM ID - 3661291.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DisableSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  ASSERT_TRUE(GetClient(1)->DisableSyncForAllDatatypes());
  AddProfile(0, CreateAutofillProfile(PROFILE_FRASIER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitFullSyncCompletion("Added a profile."));
  ASSERT_FALSE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());
  ASSERT_EQ(1U, GetAllProfiles(1).size());

  ASSERT_TRUE(GetClient(1)->EnableSyncForAllDatatypes());
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(2U, GetAllProfiles(0).size());
}

// TCM ID - 3608295.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  string16 max_length_string(AutofillTable::kMaxDataLength, '.');
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_LAST),
                max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::ADDRESS_HOME_LINE1),
                max_length_string);

  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, ExceedsMaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  string16 exceeds_max_length_string(AutofillTable::kMaxDataLength + 1, '.');
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_LAST),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllProfiles(0)[0]->guid(),
                AutofillType(autofill::ADDRESS_HOME_LINE1),
                exceeds_max_length_string);

  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(ProfilesMatch(0, 1));
}

// Test credit cards don't sync.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, NoCreditCardSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));

  CreditCard card;
  card.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                  ASCIIToUTF16("6011111111111117"));
  std::vector<CreditCard> credit_cards;
  credit_cards.push_back(card);
  SetCreditCards(0, &credit_cards);

  MakeABookmarkChange(0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfilesMatch(0, 1));
  ASSERT_EQ(1U, GetAllProfiles(0).size());

  PersonalDataManager* pdm = GetPersonalDataManager(1);
  ASSERT_EQ(0U, pdm->credit_cards().size());
}
