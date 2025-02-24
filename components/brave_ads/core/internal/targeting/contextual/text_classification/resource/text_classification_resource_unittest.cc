/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  bool LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationResourceTest, IsNotInitialized) {
  // Act & Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                             kTextClassificationResourceId));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadFileResource(kTextClassificationResourceId,
                                             ::testing::_, ::testing::_))
      .WillByDefault(::testing::Invoke(
          [](const std::string& /*id=*/, const int /*version=*/,
             LoadFileCallback callback) {
            const base::FilePath path =
                GetFileResourcePath().AppendASCII(kMissingResourceId);

            base::File file(path, base::File::Flags::FLAG_OPEN |
                                      base::File::Flags::FLAG_READ);
            std::move(callback).Run(std::move(file));
          }));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenLocaleDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  OptOutOfNotificationAdsForTesting();

  ASSERT_FALSE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenOptedInToNotificationAdsPrefDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  OptOutOfNotificationAdsForTesting();

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Act & Assert
  EXPECT_TRUE(LoadResource(kLanguageComponentId));
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidLanguageComponentId) {
  // Act & Assert
  EXPECT_FALSE(LoadResource(kInvalidLanguageComponentId));
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfOptedOutOfNotificationAds) {
  // Arrange
  OptOutOfNotificationAdsForTesting();

  // Act & Assert
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenDidUpdateResourceComponent) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act & Assert
  EXPECT_TRUE(LoadResource(kLanguageComponentId));
}

}  // namespace brave_ads
