/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test=*/true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);
  }

  void TriggerSearchResultAdEvent(
      mojom::SearchResultAdInfoPtr ad_mojom,
      const mojom::SearchResultAdEventType& event_type,
      const bool should_fire_event) {
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event));
    GetAds().TriggerSearchResultAdEvent(std::move(ad_mojom), event_type,
                                        callback.Get());
  }

  void TriggerSearchResultAdEvents(
      mojom::SearchResultAdInfoPtr ad_mojom,
      const std::vector<mojom::SearchResultAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      TriggerSearchResultAdEvent(ad_mojom->Clone(), event_type,
                                 should_fire_event);
    }
  }
};

TEST_F(BraveAdsSearchResultAdIntegrationTest, TriggerViewedEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  // Act & Assert
  TriggerSearchResultAdEvent(
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  TriggerSearchResultAdEvent(
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);
}

TEST_F(BraveAdsSearchResultAdIntegrationTest, TriggerQueuedViewedEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  SearchResultAd::DeferTriggeringOfAdViewedEvent();

  TriggerSearchResultAdEvent(
      // This ad viewed event triggering will be deferred.
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  // Act & Assert
  TriggerSearchResultAdEvent(
      // This ad viewed event will be queued as the previous ad viewed event has
      // not completed.
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  SearchResultAd::TriggerDeferredAdViewedEvent();
}

TEST_F(BraveAdsSearchResultAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  const mojom::SearchResultAdInfoPtr search_result_ad =
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true);

  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kViewed,
                             /*should_fire=*/true);

  // Act & Assert
  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kClicked,
                             /*should_fire=*/true);
}

TEST_F(BraveAdsSearchResultAdIntegrationTest,
       TriggerViewedEventsForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  DisableBraveRewardsForTesting();

  // Act & Assert
  TriggerSearchResultAdEvent(
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  TriggerSearchResultAdEvent(
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);
}

TEST_F(
    BraveAdsSearchResultAdIntegrationTest,
    DoNotTriggerViewedEventIfShouldNotAlwaysTriggerAdEventsAndBraveRewardsAreDisabled) {
  // Arrange
  DisableBraveRewardsForTesting();

  // Act & Assert
  TriggerSearchResultAdEvent(
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/false);
}

TEST_F(BraveAdsSearchResultAdIntegrationTest,
       TriggerQueuedViewedEventsForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  DisableBraveRewardsForTesting();

  SearchResultAd::DeferTriggeringOfAdViewedEvent();

  TriggerSearchResultAdEvent(
      // This ad viewed event triggering will be deferred.
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  // Act & Assert
  TriggerSearchResultAdEvent(
      // This ad viewed event will be queued as the previous ad viewed event has
      // not completed.
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewed,
      /*should_fire=*/true);

  SearchResultAd::TriggerDeferredAdViewedEvent();
}

TEST_F(BraveAdsSearchResultAdIntegrationTest,
       TriggerClickedEventForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  DisableBraveRewardsForTesting();

  const mojom::SearchResultAdInfoPtr search_result_ad =
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true);

  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kViewed,
                             /*should_fire=*/true);

  // Act & Assert
  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kClicked,
                             /*should_fire=*/true);
}

TEST_F(
    BraveAdsSearchResultAdIntegrationTest,
    DoNotTriggerClickedEventIfShouldNotAlwaysTriggerAdEventsAndBraveRewardsAreDisabled) {
  // Arrange
  DisableBraveRewardsForTesting();

  const mojom::SearchResultAdInfoPtr search_result_ad =
      BuildSearchResultAdForTesting(/*should_use_random_uuids=*/true);

  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kViewed,
                             /*should_fire=*/false);

  // Act & Assert
  TriggerSearchResultAdEvent(search_result_ad.Clone(),
                             mojom::SearchResultAdEventType::kClicked,
                             /*should_fire=*/false);
}

}  // namespace brave_ads
