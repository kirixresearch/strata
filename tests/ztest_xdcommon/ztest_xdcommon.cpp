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

			unsigned char control[7] = { 0xff, 0x01, 0x03, 1, 2, 3, 0x00 };

			unsigned char* test;
			size_t len;
			test = (unsigned char*)r.serialize(&len);

			Assert::AreEqual((size_t)7, len);
			Assert::AreEqual(0, memcmp(test, control, 7));
		}
	};
}
