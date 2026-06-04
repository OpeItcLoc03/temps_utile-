// Host unit tests for the EEPROM storage version-invalidation contract.
//
// Step 5 of the Teensy 4.0 port shrinks the calibration block and bumps storage
// versions so stale EEPROM slots are discarded (fresh start, no migration).
// The mechanism that makes that safe lives in PageStorage::Load:
//   - reject a page when its FOURCC differs from the data type's FOURCC, OR
//   - reject a page when its stored size differs from sizeof(DATA_TYPE).
// On rejection, Load returns false and the caller initialises defaults.
//
// These tests pin that contract on the host with a fake STORAGE backend, so a
// refactor that silently weakens the version check becomes visible. The
// production CalibrationData FOURCC bump (CAL,1 -> CAL,2) is the direct
// application of the contract pinned here.

#include <string.h>
#include <stdint.h>
#include <unity.h>

#include "src/util_pagestorage.h"

// --- Fake STORAGE backend: a flat byte array, same static interface PageStorage
//     expects (read / write / update / LENGTH). ----------------------------
struct FakeStorage {
  static const size_t LENGTH = 2048;
  static uint8_t buf[LENGTH];

  static void update(size_t addr, const void *data, size_t length) {
    memcpy(buf + addr, data, length);
  }
  static void write(size_t addr, const void *data, size_t length) {
    memcpy(buf + addr, data, length);
  }
  static void read(size_t addr, void *data, size_t length) {
    memcpy(data, buf + addr, length);
  }
  static void clear() { memset(buf, 0, LENGTH); }
};
uint8_t FakeStorage::buf[FakeStorage::LENGTH];

// --- Representative data types modelling the calibration block before/after a
//     version bump. Mirror the real CalibrationData FOURCC idiom. ----------
struct CalV1 {                       // "old" layout, version 1
  static constexpr uint32_t FOURCC = ::FOURCC<'C', 'A', 'L', 1>::value;
  uint16_t calibration_points[1][5]; // 5-point DAC LUT (removed in v2)
  uint8_t  display_offset;
};

struct CalV2 {                       // "new" layout, version bumped to 2
  static constexpr uint32_t FOURCC = ::FOURCC<'C', 'A', 'L', 2>::value;
  uint16_t calibration_points[1][3]; // shrunk: DAC LUT gone
  uint8_t  display_offset;
};

struct CalV1Resized {                // same FOURCC as V1, different size
  static constexpr uint32_t FOURCC = ::FOURCC<'C', 'A', 'L', 1>::value;
  uint16_t calibration_points[1][2];
};

using StoreV1        = PageStorage<FakeStorage, 0, 128, CalV1>;
using StoreV2        = PageStorage<FakeStorage, 0, 128, CalV2>;
using StoreV1Resized = PageStorage<FakeStorage, 0, 128, CalV1Resized>;

void setUp(void) { FakeStorage::clear(); }
void tearDown(void) {}

// A round-trip with the SAME version must restore identical data.
static void test_roundtrip_same_version(void) {
  CalV1 in;
  in.calibration_points[0][0] = 0x1111;
  in.calibration_points[0][4] = 0x4444;
  in.display_offset = 0x42;

  StoreV1 writer;
  writer.Init();
  TEST_ASSERT_TRUE(writer.Save(in));

  CalV1 out;
  memset(&out, 0, sizeof(out));
  StoreV1 reader;
  TEST_ASSERT_TRUE(reader.Load(out));
  TEST_ASSERT_EQUAL_HEX16(0x1111, out.calibration_points[0][0]);
  TEST_ASSERT_EQUAL_HEX16(0x4444, out.calibration_points[0][4]);
  TEST_ASSERT_EQUAL_HEX8(0x42, out.display_offset);
}

// Bumping the FOURCC (CAL,1 -> CAL,2) must invalidate every old-version page:
// Load returns false so the caller falls back to defaults. This is the step-5
// "fresh start, no migration" guarantee.
static void test_fourcc_bump_invalidates(void) {
  CalV1 in;
  memset(&in, 0xAB, sizeof(in));
  StoreV1 writer;
  writer.Init();
  TEST_ASSERT_TRUE(writer.Save(in));

  CalV2 out;
  memset(&out, 0, sizeof(out));
  StoreV2 reader;                       // same address range, bumped FOURCC
  TEST_ASSERT_FALSE(reader.Load(out));  // old page rejected -> defaults
}

// Shrinking the struct without a FOURCC bump is still caught: the size guard
// rejects a page whose stored size != sizeof(new type).
static void test_size_mismatch_invalidates(void) {
  CalV1 in;
  memset(&in, 0x5A, sizeof(in));
  StoreV1 writer;
  writer.Init();
  TEST_ASSERT_TRUE(writer.Save(in));

  CalV1Resized out;
  memset(&out, 0, sizeof(out));
  StoreV1Resized reader;                // same FOURCC, different sizeof
  TEST_ASSERT_FALSE(reader.Load(out));
}

// Fresh/empty storage yields no valid page -> Load false (defaults path).
static void test_empty_storage_load_false(void) {
  CalV1 out;
  memset(&out, 0, sizeof(out));
  StoreV1 reader;
  TEST_ASSERT_FALSE(reader.Load(out));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_roundtrip_same_version);
  RUN_TEST(test_fourcc_bump_invalidates);
  RUN_TEST(test_size_mismatch_invalidates);
  RUN_TEST(test_empty_storage_load_false);
  return UNITY_END();
}
