/* vim:set ts=2 sw=2 sts=0 et: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include "gtest/gtest.h"
#include "js/Conversions.h"
#include "nsITelemetry.h"
#include "Telemetry.h"
#include "TelemetryFixture.h"

using namespace mozilla;

namespace {

void
GetAndClearHistogram(JSContext* cx, nsCOMPtr<nsITelemetry> mTelemetry,
                     const nsACString &name, bool is_keyed)
{
  JS::RootedValue testHistogram(cx);
  nsresult rv = is_keyed ? mTelemetry->GetKeyedHistogramById(name, cx, &testHistogram)
                         : mTelemetry->GetHistogramById(name, cx, &testHistogram);

  ASSERT_EQ(rv, NS_OK) << "Cannot fetch histogram";

  // Clear the stored value
  JS::RootedObject testHistogramObj(cx, &testHistogram.toObject());
  JS::RootedValue rval(cx);
  ASSERT_TRUE(JS_CallFunctionName(cx, testHistogramObj, "clear",
                  JS::HandleValueArray::empty(), &rval)) << "Cannot clear histogram";
}

void
GetProperty(JSContext* cx, const char* name, JS::HandleValue valueIn,
            JS::MutableHandleValue valueOut)
{
  JS::RootedValue property(cx);
  JS::RootedObject valueInObj(cx, &valueIn.toObject());
  ASSERT_TRUE(JS_GetProperty(cx, valueInObj, name, &property))
    << "Cannot get property '" << name << "'";
  valueOut.set(property);
}

void
GetElement(JSContext* cx, uint32_t index, JS::HandleValue valueIn,
           JS::MutableHandleValue valueOut)
{
  JS::RootedValue element(cx);
  JS::RootedObject valueInObj(cx, &valueIn.toObject());
  ASSERT_TRUE(JS_GetElement(cx, valueInObj, index, &element))
    << "Cannot get element at index '" << index << "'";
  valueOut.set(element);
}

void
GetSnapshots(JSContext* cx, nsCOMPtr<nsITelemetry> mTelemetry,
             const char* name, JS::MutableHandleValue valueOut, bool is_keyed)
{
  JS::RootedValue snapshots(cx);
  nsresult rv = is_keyed ? mTelemetry->SnapshotKeyedHistograms(nsITelemetry::DATASET_RELEASE_CHANNEL_OPTIN, false, false, cx, &snapshots)
                         : mTelemetry->SnapshotHistograms(nsITelemetry::DATASET_RELEASE_CHANNEL_OPTIN, false, false, cx, &snapshots);

  JS::RootedValue snapshot(cx);
  GetProperty(cx, "parent", snapshots, &snapshot);

  ASSERT_EQ(rv, NS_OK) << "Cannot call histogram snapshots";
  valueOut.set(snapshot);
}

}

TEST_F(TelemetryTestFixture, AccumulateCountHistogram)
{
  const uint32_t kExpectedValue = 100;
  AutoJSContextWithGlobal cx(mCleanGlobal);

  GetAndClearHistogram(cx.GetJSContext(), mTelemetry, NS_LITERAL_CSTRING("TELEMETRY_TEST_COUNT"),
                       false);

  // Accumulate in the histogram
  Telemetry::Accumulate(Telemetry::TELEMETRY_TEST_COUNT, kExpectedValue);

  // Get a snapshot for all the histograms
  JS::RootedValue snapshot(cx.GetJSContext());
  GetSnapshots(cx.GetJSContext(), mTelemetry, "TELEMETRY_TEST_COUNT", &snapshot, false);

  // Get the histogram from the snapshot
  JS::RootedValue histogram(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "TELEMETRY_TEST_COUNT", snapshot, &histogram);

  // Get "sum" property from histogram
  JS::RootedValue sum(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "sum", histogram,  &sum);

  // Check that the "sum" stored in the histogram matches with |kExpectedValue|
  uint32_t uSum = 0;
  JS::ToUint32(cx.GetJSContext(), sum, &uSum);
  ASSERT_EQ(uSum, kExpectedValue) << "The histogram is not returning expected value";
}

TEST_F(TelemetryTestFixture, AccumulateKeyedCountHistogram)
{
  const uint32_t kExpectedValue = 100;
  AutoJSContextWithGlobal cx(mCleanGlobal);

  GetAndClearHistogram(cx.GetJSContext(), mTelemetry,
                       NS_LITERAL_CSTRING("TELEMETRY_TEST_KEYED_COUNT"), true);

  // Accumulate data in the provided key within the histogram
  Telemetry::Accumulate(Telemetry::TELEMETRY_TEST_KEYED_COUNT, NS_LITERAL_CSTRING("sample"),
                        kExpectedValue);

  // Get a snapshot for all the histograms
  JS::RootedValue snapshot(cx.GetJSContext());
  GetSnapshots(cx.GetJSContext(), mTelemetry, "TELEMETRY_TEST_KEYED_COUNT", &snapshot, true);

  // Get the histogram from the snapshot
  JS::RootedValue histogram(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "TELEMETRY_TEST_KEYED_COUNT", snapshot, &histogram);

  // Get "sample" property from histogram
  JS::RootedValue expectedKeyData(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "sample", histogram,  &expectedKeyData);

  // Get "sum" property from keyed data
  JS::RootedValue sum(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "sum", expectedKeyData,  &sum);

  // Check that the sum stored in the histogram matches with |kExpectedValue|
  uint32_t uSum = 0;
  JS::ToUint32(cx.GetJSContext(), sum, &uSum);
  ASSERT_EQ(uSum, kExpectedValue) << "The histogram is not returning expected sum";
}

TEST_F(TelemetryTestFixture, TestKeyedKeysHistogram)
{
  AutoJSContextWithGlobal cx(mCleanGlobal);

  JS::RootedValue testHistogram(cx.GetJSContext());
  JS::RootedValue rval(cx.GetJSContext());

  GetAndClearHistogram(cx.GetJSContext(), mTelemetry,
                       NS_LITERAL_CSTRING("TELEMETRY_TEST_KEYED_KEYS"), true);

  // Test the accumulation on both the allowed and unallowed keys, using
  // the API that accepts histogram IDs.
  Telemetry::Accumulate(Telemetry::TELEMETRY_TEST_KEYED_KEYS, NS_LITERAL_CSTRING("not-allowed"), 1);
  Telemetry::Accumulate(Telemetry::TELEMETRY_TEST_KEYED_KEYS, NS_LITERAL_CSTRING("testkey"), 0);
  // Do the same, using the API that accepts the histogram name as a string.
  Telemetry::Accumulate("TELEMETRY_TEST_KEYED_KEYS", NS_LITERAL_CSTRING("not-allowed"), 1);
  Telemetry::Accumulate("TELEMETRY_TEST_KEYED_KEYS", NS_LITERAL_CSTRING("CommonKey"), 1);

  // Get a snapshot for all the histograms
  JS::RootedValue snapshot(cx.GetJSContext());
  GetSnapshots(cx.GetJSContext(), mTelemetry, "TELEMETRY_TEST_KEYED_KEYS", &snapshot, true);

  // Get the histogram from the snapshot
  JS::RootedValue histogram(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "TELEMETRY_TEST_KEYED_KEYS", snapshot, &histogram);

  // Get "testkey" property from histogram and check that it stores the correct
  // data.
  JS::RootedValue expectedKeyData(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "testkey", histogram,  &expectedKeyData);
  ASSERT_TRUE(!expectedKeyData.isUndefined())
    << "Cannot find the expected key in the histogram data";
  JS::RootedValue sum(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "sum", expectedKeyData,  &sum);
  uint32_t uSum = 0;
  JS::ToUint32(cx.GetJSContext(), sum, &uSum);
  ASSERT_EQ(uSum, 0U) << "The histogram is not returning expected sum for 'testkey'";

  // Do the same for the "CommonKey" property.
  GetProperty(cx.GetJSContext(), "CommonKey", histogram,  &expectedKeyData);
  ASSERT_TRUE(!expectedKeyData.isUndefined())
    << "Cannot find the expected key in the histogram data";
  GetProperty(cx.GetJSContext(), "sum", expectedKeyData,  &sum);
  JS::ToUint32(cx.GetJSContext(), sum, &uSum);
  ASSERT_EQ(uSum, 1U) << "The histogram is not returning expected sum for 'CommonKey'";

  GetProperty(cx.GetJSContext(), "not-allowed", histogram,  &expectedKeyData);
  ASSERT_TRUE(expectedKeyData.isUndefined())
    << "Unallowed keys must not be recorded in the histogram data";
}

TEST_F(TelemetryTestFixture, AccumulateCategoricalHistogram)
{
  const uint32_t kExpectedValue = 2;

  AutoJSContextWithGlobal cx(mCleanGlobal);

  GetAndClearHistogram(cx.GetJSContext(), mTelemetry,
                       NS_LITERAL_CSTRING("TELEMETRY_TEST_CATEGORICAL"), false);

  // Accumulate one unit into the categorical histogram with label
  // Telemetry::LABELS_TELEMETRY_TEST_CATEGORICAL::CommonLabel
  Telemetry::AccumulateCategorical(Telemetry::LABELS_TELEMETRY_TEST_CATEGORICAL::CommonLabel);

  // Accumulate another unit into the same categorical histogram using a string label
  Telemetry::AccumulateCategorical(Telemetry::TELEMETRY_TEST_CATEGORICAL,
                                   NS_LITERAL_CSTRING("CommonLabel"));

  // Get a snapshot for all the histograms
  JS::RootedValue snapshot(cx.GetJSContext());
  GetSnapshots(cx.GetJSContext(), mTelemetry, "TELEMETRY_TEST_CATEGORICAL", &snapshot, false);

  // Get our histogram from the snapshot
  JS::RootedValue histogram(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "TELEMETRY_TEST_CATEGORICAL", snapshot, &histogram);

  // Get counts array from histogram. Each entry in the array maps to a label in the histogram.
  JS::RootedValue counts(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "counts", histogram,  &counts);

  // Get the value for the label we care about
  JS::RootedValue value(cx.GetJSContext());
  GetElement(cx.GetJSContext(),
             static_cast<uint32_t>(Telemetry::LABELS_TELEMETRY_TEST_CATEGORICAL::CommonLabel),
             counts, &value);

  // Check that the value stored in the histogram matches with |kExpectedValue|
  uint32_t uValue = 0;
  JS::ToUint32(cx.GetJSContext(), value, &uValue);
  ASSERT_EQ(uValue, kExpectedValue) << "The histogram is not returning expected value";
}

TEST_F(TelemetryTestFixture, AccumulateKeyedCategoricalHistogram)
{
  const uint32_t kSampleExpectedValue = 2;
  const uint32_t kOtherSampleExpectedValue = 1;

  AutoJSContextWithGlobal cx(mCleanGlobal);

  GetAndClearHistogram(cx.GetJSContext(), mTelemetry,
                       NS_LITERAL_CSTRING("TELEMETRY_TEST_KEYED_CATEGORICAL"), true);

  // Accumulate one unit into the categorical histogram with label
  // Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel
  Telemetry::AccumulateCategoricalKeyed(NS_LITERAL_CSTRING("sample"),
                                        Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel);
  // Accumulate another unit into the same categorical histogram
  Telemetry::AccumulateCategoricalKeyed(NS_LITERAL_CSTRING("sample"),
                                        Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel);
  // Accumulate another unit into a different categorical histogram
  Telemetry::AccumulateCategoricalKeyed(NS_LITERAL_CSTRING("other-sample"),
                                        Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel);

  // Get a snapshot for all the histograms
  JS::RootedValue snapshot(cx.GetJSContext());
  GetSnapshots(cx.GetJSContext(), mTelemetry, "TELEMETRY_TEST_KEYED_CATEGORICAL", &snapshot, true);
  // Get the histogram from the snapshot
  JS::RootedValue histogram(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "TELEMETRY_TEST_KEYED_CATEGORICAL", snapshot, &histogram);

  // Check that the sample histogram contains the values we expect
  JS::RootedValue sample(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "sample", histogram,  &sample);
  // Get counts array from the sample. Each entry in the array maps to a label in the histogram.
  JS::RootedValue sampleCounts(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "counts", sample,  &sampleCounts);
  // Get the value for the label we care about
  JS::RootedValue sampleValue(cx.GetJSContext());
  GetElement(cx.GetJSContext(),
             static_cast<uint32_t>(Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel),
             sampleCounts, &sampleValue);
  // Check that the value stored in the histogram matches with |kSampleExpectedValue|
  uint32_t uSampleValue = 0;
  JS::ToUint32(cx.GetJSContext(), sampleValue, &uSampleValue);
  ASSERT_EQ(uSampleValue, kSampleExpectedValue) << "The sample histogram is not returning expected value";

  // Check that the other-sample histogram contains the values we expect
  JS::RootedValue otherSample(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "other-sample", histogram,  &otherSample);
  // Get counts array from the other-sample. Each entry in the array maps to a label in the histogram.
  JS::RootedValue otherCounts(cx.GetJSContext());
  GetProperty(cx.GetJSContext(), "counts", otherSample,  &otherCounts);
  // Get the value for the label we care about
  JS::RootedValue otherValue(cx.GetJSContext());
  GetElement(cx.GetJSContext(),
             static_cast<uint32_t>(Telemetry::LABELS_TELEMETRY_TEST_KEYED_CATEGORICAL::CommonLabel),
             otherCounts, &otherValue);
  // Check that the value stored in the histogram matches with |kOtherSampleExpectedValue|
  uint32_t uOtherValue = 0;
  JS::ToUint32(cx.GetJSContext(), otherValue, &uOtherValue);
  ASSERT_EQ(uOtherValue, kOtherSampleExpectedValue) << "The other-sample histogram is not returning expected value";
}
