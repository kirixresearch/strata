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
		TEST_METHOD(TestLocalRowSerialize)
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

			LocalRowValue v3;
			v3.setNull();
			r.setColumnData(2, v3);

			unsigned char control[8] = { 0xff, 0x01, 0x03, 1, 2, 3, 0xff, 0x00 };
			size_t control_size = sizeof(control);

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual(control_size, len);
			Assert::AreEqual(0, memcmp(test, control, control_size));
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



	};
}
