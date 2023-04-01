#include "pch.h"
#include "CppUnitTest.h"
#include "../../xd/xdcommon/localrowcache2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ztestxdcommon
{
	TEST_CLASS(ztestxdcommon)
	{
	public:
		
		TEST_METHOD(TestLocalRowValueSetGetData)
		{
			LocalRowValue v;
			unsigned char buf[3] = { 1, 2, 3 };
			v.setData(buf, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf, 3));
		}

		TEST_METHOD(TestLocalRowValueGetDataLength)
		{
			LocalRowValue v;
			unsigned char buf[3] = { 1, 2, 3 };
			v.setData(buf, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf, 3));
			Assert::AreEqual((size_t)3, v.getDataLength());
		}

		TEST_METHOD(TestLocalRowValueGetType)
		{
			LocalRowValue v;
			v.setType(LocalRowValue::typeBoolean);
			Assert::AreEqual((unsigned char)LocalRowValue::typeBoolean, v.getType());
		}
		TEST_METHOD(TestLocalRowValueSetGetDataMultiple)
		{
			LocalRowValue v;

			unsigned char buf1[3] = { 1, 2, 3 };
			v.setData(buf1, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf1, 3));

			unsigned char buf2[3] = { 3, 2, 1 };
			v.setData(buf2, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf2, 3));

			unsigned char buf3[4] = { 1, 2, 3, 4 };
			v.setData(buf3, 4);
			Assert::AreEqual(0, memcmp(v.getData(), buf3, 4));
		}


		TEST_METHOD(TestLocalRowValueSetGetNull)
		{
			LocalRowValue v;

			v.setNull();
			Assert::AreEqual(true, v.isNull());

			unsigned char buf1[3] = { 1, 2, 3 };
			v.setData(buf1, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf1, 3));
			Assert::AreEqual(false, v.isNull());
		}

		TEST_METHOD(TestLocalRowValueSetNullAfterData)
		{
			LocalRowValue v;

			unsigned char buf1[3] = { 1, 2, 3 };
			v.setData(buf1, 3);
			Assert::AreEqual(0, memcmp(v.getData(), buf1, 3));
			Assert::AreEqual(false, v.isNull());

			v.setNull();
			Assert::AreEqual(true, v.isNull());
			Assert::IsNull(v.getData());
			Assert::AreEqual((size_t)0, v.getDataLength());
		}


		TEST_METHOD(TestLocalRowGetColumnData)
		{
			LocalRow2 r;

			LocalRowValue v1;
			v1.setNull();
			r.setColumnData(0, v1);

			LocalRowValue v2;
			unsigned char buf1[3] = { 1, 2, 3 };
			v2.setData(buf1, 3);
			r.setColumnData(1, v2);

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = r.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, 3));
		}

		TEST_METHOD(TestLocalRowAutoResize)
		{
			LocalRow2 r;

			Assert::AreEqual((size_t)0, r.getColumnCount());

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual((size_t)1, r.getColumnCount());

			LocalRowValue& t2 = r.getColumnData(999);
			Assert::AreEqual((size_t)1000, r.getColumnCount());

			LocalRowValue& t3 = r.getColumnData(500);
			Assert::AreEqual((size_t)1000, r.getColumnCount());
			Assert::IsTrue(t2.isNull());
		}

		TEST_METHOD(TestLocalRowSerialize)
		{
			LocalRow2 r;

			LocalRowValue v0;
			v0.setNull();
			r.setColumnData(0, v0);

			LocalRowValue v1;
			unsigned char buf1[3] = { 1, 2, 3 };
			v1.setData(buf1, 3);
			r.setColumnData(1, v1);

			LocalRowValue v2;
			v2.setNull();
			r.setColumnData(2, v2);

			LocalRowValue v3;
			v3.setNull();
			v3.setType(LocalRowValue::typeBoolean);
			r.setColumnData(3, v3);

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = r.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, 3));


			unsigned char control[9] = { 0x05, 0x01, 0x03, 1, 2, 3, 0x05, (LocalRowValue::typeBoolean << 3) | 0x05, 0x00 };
			size_t control_size = sizeof(control);

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual(control_size, len);
			Assert::AreEqual(0, memcmp(test, control, control_size));
		}


		TEST_METHOD(TestLocalRowSerialize16it)
		{
			LocalRow2 r;

			LocalRowValue v1;
			v1.setNull();
			r.setColumnData(0, v1);

			LocalRowValue v2;
			size_t test_size = 260;
			unsigned char* buf1 = new unsigned char[test_size]; // 24-bit size
			memset(buf1, 1, test_size);
			v2.setData(buf1, test_size);
			r.setColumnData(1, v2);

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = r.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, sizeof(buf1)));

			LocalRowValue v3;
			v3.setNull();
			r.setColumnData(2, v3);

			size_t control_size = test_size + 6;
			unsigned char* control = new unsigned char[control_size];
			memset(control, 1, control_size);
			control[0] = 0x05;
			control[1] = 0x02; // three bytes for size
			control[2] = (test_size >> 8) & 0xff;
			control[3] = (test_size >> 0) & 0xff;
			control[control_size - 2] = 0x05;
			control[control_size - 1] = 0x00;

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual(control_size, len);
			Assert::AreEqual(0, memcmp(test, control, control_size));

			delete[] control;
			delete[] buf1;
		}

		TEST_METHOD(TestLocalRowSerialize24bit)
		{
			LocalRow2 r;

			LocalRowValue v1;
			v1.setNull();
			r.setColumnData(0, v1);

			LocalRowValue v2;
			size_t test_size = 65570;
			unsigned char* buf1 = new unsigned char[test_size]; // 24-bit size
			memset(buf1, 1, test_size);
			v2.setData(buf1, test_size);
			r.setColumnData(1, v2);

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = r.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, sizeof(buf1)));

			LocalRowValue v3;
			v3.setNull();
			r.setColumnData(2, v3);

			size_t control_size = test_size + 7;
			unsigned char* control = new unsigned char[control_size];
			memset(control, 1, control_size);
			control[0] = 0x05;
			control[1] = 0x03; // three bytes for size
			control[2] = (test_size >> 16) & 0xff;
			control[3] = (test_size >> 8) & 0xff;
			control[4] = (test_size >> 0) & 0xff;
			control[control_size-2] = 0x05;
			control[control_size-1] = 0x00;

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual(control_size, len);
			Assert::AreEqual(0, memcmp(test, control, control_size));

			delete[] control;
			delete[] buf1;
		}

		TEST_METHOD(TestLocalRowSerialize32bit)
		{
			LocalRow2 r;

			LocalRowValue v1;
			v1.setNull();
			r.setColumnData(0, v1);

			LocalRowValue v2;
			size_t test_size = 0x1000004;
			unsigned char* buf1 = new unsigned char[test_size]; // 24-bit size
			memset(buf1, 1, test_size);
			v2.setData(buf1, test_size);
			r.setColumnData(1, v2);

			LocalRowValue& t1 = r.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = r.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, sizeof(buf1)));

			LocalRowValue v3;
			v3.setNull();
			r.setColumnData(2, v3);

			size_t control_size = test_size + 8;
			unsigned char* control = new unsigned char[control_size];
			memset(control, 1, control_size);
			control[0] = 0x05;
			control[1] = 0x04; // four bytes for size
			control[2] = (test_size >> 24) & 0xff;
			control[3] = (test_size >> 16) & 0xff;
			control[4] = (test_size >> 8) & 0xff;
			control[5] = (test_size >> 0) & 0xff;
			control[control_size - 2] = 0x05;
			control[control_size - 1] = 0x00;

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual(control_size, len);
			Assert::AreEqual(0, memcmp(test, control, control_size));

			delete[] control;
			delete[] buf1;
		}

		TEST_METHOD(TestLocalRowSerializeUnserialize1)
		{
			LocalRow2 row1, row2;

			LocalRowValue v1;
			v1.setNull();
			row1.setColumnData(0, v1);

			LocalRowValue v2;
			unsigned char buf1[3] = { 1, 2, 3 };
			v2.setData(buf1, 3);
			row1.setColumnData(1, v2);

			LocalRowValue v3;
			v3.setNull();
			row1.setColumnData(2, v3);

			size_t size = 0;
			const unsigned char* data = row1.serialize(&size);

			row2.unserialize(data, size);

			LocalRowValue& t1 = row2.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = row2.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, 3));

			LocalRowValue& t3 = row2.getColumnData(2);
			Assert::AreEqual(true, t3.isNull());

			Assert::AreEqual((size_t)3, row2.getColumnCount());
		}


		TEST_METHOD(TestLocalRowCorrectColumnCountAfterUnserialize)
		{
			LocalRow2 row1, row2;

			LocalRowValue v1;
			v1.setNull();
			row1.setColumnData(0, v1);

			LocalRowValue v2;
			unsigned char buf1[3] = { 1, 2, 3 };
			v2.setData(buf1, 3);
			row1.setColumnData(1, v2);

			size_t size = 0;
			const unsigned char* data = row1.serialize(&size);

			row2.setColumnData(0, v1);
			row2.setColumnData(1, v1);
			row2.setColumnData(2, v1);
			row2.setColumnData(3, v1);
			row2.setColumnData(4, v1);

			row2.unserialize(data, size);

			LocalRowValue& t1 = row2.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = row2.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, 3));

			Assert::AreEqual((size_t)2, row2.getColumnCount());
		}

		TEST_METHOD(TestLocalRowCacheSetGetRow)
		{
			LocalRow2 row1, row2;

			LocalRowValue v1;
			v1.setNull();
			row1.setColumnData(0, v1);

			LocalRowValue v2;
			unsigned char buf1[3] = { 1, 2, 3 };
			v2.setData(buf1, 3);
			row1.setColumnData(1, v2);

			LocalRowValue v3;
			v3.setNull();
			row1.setColumnData(2, v3);

			LocalRowCache2 cache;
			cache.putRow(1, row1);
			cache.getRow(1, row2);

			LocalRowValue& t1 = row2.getColumnData(0);
			Assert::AreEqual(true, t1.isNull());

			LocalRowValue& t2 = row2.getColumnData(1);
			Assert::AreEqual(0, memcmp(t2.getData(), buf1, 3));

			LocalRowValue& t3 = row2.getColumnData(2);
			Assert::AreEqual(true, t3.isNull());

			Assert::AreEqual((size_t)3, row2.getColumnCount());
		}

		TEST_METHOD(TestLocalRowCacheGetNonexistantRow)
		{
			LocalRowCache2 cache;
			LocalRow2 row1, row2;

			LocalRowValue v1;
			v1.setNull();
			row1.setColumnData(0, v1);

			cache.putRow(1, row1);

			Assert::IsFalse(cache.getRow(50, row2));
			Assert::IsTrue(cache.getRow(1, row2));
		}

		TEST_METHOD(TestLocalRowCacheSetGetRow5000)
		{
			uint32_t number_of_test_rows;

			// once in memory, once on disk
			for (int is_disk = 0; is_disk < 2; ++is_disk)
			{
				LocalRowCache2 cache;

				if (is_disk)
				{
					number_of_test_rows = 500;

					char path[255];
					tmpnam_s(path, 254);
					std::string spath = path;
					std::wstring wpath(spath.begin(), spath.end());
					bool result = cache.init(wpath);
					Assert::IsTrue(result);
				}
				else
				{
					number_of_test_rows = 5000;
				}

				for (uint32_t i = 0; i < number_of_test_rows; ++i)
				{
					LocalRow2 row;

					LocalRowValue v0;
					unsigned char rand[255];
					memset(rand, i % 255, sizeof(rand));
					v0.setData(rand, i % 255);
					row.setColumnData(0, v0);

					LocalRowValue v1;
					unsigned char data[sizeof(uint32_t)];
					memcpy(data, &i, sizeof(uint32_t));
					v1.setData(data, sizeof(uint32_t));
					row.setColumnData(1, v1);

					cache.putRow(i, row);
				}

				for (uint32_t i = 0; i < number_of_test_rows; ++i)
				{
					uint32_t desired_row_num = rand() % number_of_test_rows;

					LocalRow2 row;
					cache.getRow(desired_row_num, row);

					LocalRowValue& v = row.getColumnData(1);

					uint32_t check_row_num;
					memcpy(&check_row_num, v.getData(), sizeof(uint32_t));

					Assert::AreEqual(desired_row_num, check_row_num);
				}
			}
		}
	};
}
