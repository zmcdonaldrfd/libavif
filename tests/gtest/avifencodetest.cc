// Copyright 2023 Google LLC
// SPDX-License-Identifier: BSD-2-Clause

#include <cstdint>
#include <iostream>

#include "avif/avif.h"
#include "aviftest_helpers.h"
#include "gtest/gtest.h"

namespace avif {
namespace {

// Used to pass the data folder path to the GoogleTest suites.
const char* data_path = nullptr;

//------------------------------------------------------------------------------

TEST(BasicTest, Encode) {
  ImagePtr image = testutil::ReadImage(data_path, "paris_exif_xmp_icc.jpg");
  ASSERT_NE(image, nullptr);

  EncoderPtr encoder(avifEncoderCreate());
  ASSERT_NE(encoder, nullptr);
  testutil::AvifRwData encoded;
  ASSERT_EQ(avifEncoderWrite(encoder.get(), image.get(), &encoded),
            AVIF_RESULT_OK);

  // Uncomment to save the generated file to disk.
  // std::ofstream(testing::TempDir() + "avifencodetest.avif", std::ios::binary)
  //     .write(reinterpret_cast<char*>(encoded.data), encoded.size);
}

TEST(EncodeTest, SequenceDurationDoesNotFit) {
  if (!testutil::Av1EncoderAvailable()) {
    GTEST_SKIP() << "AV1 encoder unavailable, skip test.";
  }

  ImagePtr image = testutil::CreateImage(16, 16, 8, AVIF_PIXEL_FORMAT_YUV420,
                                         AVIF_PLANES_YUV);
  ASSERT_NE(image, nullptr);
  testutil::FillImageGradient(image.get());

  const auto finish_sequence = [&](uint64_t first_duration,
                                   uint64_t second_duration) {
    EncoderPtr encoder(avifEncoderCreate());
    EXPECT_NE(encoder, nullptr);
    encoder->speed = AVIF_SPEED_FASTEST;
    encoder->timescale = 1;
    encoder->repetitionCount = 0;
    EXPECT_EQ(avifEncoderAddImage(encoder.get(), image.get(), first_duration,
                                  AVIF_ADD_IMAGE_FLAG_NONE),
              AVIF_RESULT_OK);
    EXPECT_EQ(avifEncoderAddImage(encoder.get(), image.get(), second_duration,
                                  AVIF_ADD_IMAGE_FLAG_NONE),
              AVIF_RESULT_OK);
    testutil::AvifRwData encoded;
    return avifEncoderFinish(encoder.get(), &encoded);
  };

  // UINT64_MAX + 1 does not fit in the uint64_t duration sum.
  EXPECT_EQ(finish_sequence(UINT64_MAX, 1), AVIF_RESULT_INVALID_ARGUMENT);
  // sample_delta is 32-bit. The sum of these two durations fits in uint64_t.
  EXPECT_EQ(finish_sequence(static_cast<uint64_t>(UINT32_MAX) + 1,
                            static_cast<uint64_t>(UINT32_MAX) + 1),
            AVIF_RESULT_INVALID_ARGUMENT);
}

//------------------------------------------------------------------------------

}  // namespace
}  // namespace avif

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    std::cerr << "There must be exactly one argument containing the path to "
                 "the test data folder"
              << std::endl;
    return 1;
  }
  avif::data_path = argv[1];
  return RUN_ALL_TESTS();
}
