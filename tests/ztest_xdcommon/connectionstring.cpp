#include "pch.h"
#include "CppUnitTest.h"
#include "../../xd/include/xd/xd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ztestxdcommon
{
	TEST_CLASS(connectionstring)
	{
	public:
		


		TEST_METHOD(ConnectionStringValueExists)
        {
            xd::ConnectionString cstr;
            cstr.setValue(L"host", L"localhost");
            Assert::IsTrue(cstr.getValueExist(L"host"));
            Assert::IsFalse(cstr.getValueExist(L"host2"));
        }

		TEST_METHOD(ConnectionStringBasicGetAndSet)
		{
			xd::ConnectionString cstr;
			cstr.setValue(L"host", L"localhost");
			Assert::AreEqual(cstr.getValue(L"host"), std::wstring(L"localhost"));
		}

		TEST_METHOD(ConnectionStringBasicGetNotExist)
		{
			xd::ConnectionString cstr;
			cstr.setValue(L"host", L"localhost");
			Assert::AreEqual(cstr.getValue(L"host2"), std::wstring(L""));
		}

		TEST_METHOD(ConnectionStringGetValueCase)
		{
            xd::ConnectionString cstr;
            cstr.setValue(L"host", L"localhost");
            Assert::AreEqual(cstr.getValue(L"HOST"), std::wstring(L"localhost"));
        }

		TEST_METHOD(ConnectionStringGetLowerValue)
		{
			xd::ConnectionString cstr;
			cstr.setValue(L"host", L"LOCALHOST");
			Assert::AreEqual(cstr.getLowerValue(L"Host"), std::wstring(L"localhost"));
		}

		TEST_METHOD(ConnectionStringBuildConnectionString)
		{
			xd::ConnectionString cstr;
			cstr.setValue(L"host", L"localhost");
			cstr.setValue(L"port", L"1234");
			Assert::AreEqual(cstr.getConnectionString(), std::wstring(L"host=localhost;port=1234"));
		}

		TEST_METHOD(ConnectionStringBuildConnectionStringOrder)
		{
			xd::ConnectionString cstr;
			cstr.setValue(L"port", L"1234");
			cstr.setValue(L"host", L"localhost");
			Assert::AreEqual(cstr.getConnectionString(), std::wstring(L"port=1234;host=localhost"));
		}

		TEST_METHOD(ConnectionStringParseConnectionString)
		{
            xd::ConnectionString cstr1(L"host=localhost;port=1234");
            Assert::AreEqual(cstr1.getValue(L"host"), std::wstring(L"localhost"));
            Assert::AreEqual(cstr1.getValue(L"port"), std::wstring(L"1234"));

			xd::ConnectionString cstr2(L"Host=localhost;Port=1234");
			Assert::AreEqual(cstr2.getValue(L"host"), std::wstring(L"localhost"));
			Assert::AreEqual(cstr2.getValue(L"port"), std::wstring(L"1234"));

			xd::ConnectionString cstr3(L"   Host=localhost; Port=1234   ");
			Assert::AreEqual(cstr3.getValue(L"host"), std::wstring(L"localhost"));
			Assert::AreEqual(cstr3.getValue(L"port"), std::wstring(L"1234"));

			xd::ConnectionString cstr4(L"   Host = localhost; Port = 1234   ");
			Assert::AreEqual(cstr4.getValue(L"host"), std::wstring(L"localhost"));
			Assert::AreEqual(cstr4.getValue(L"port"), std::wstring(L"1234"));

			xd::ConnectionString cstr5(L"   Host = localhost  ; Port = 1234   ");
			Assert::AreEqual(cstr5.getValue(L"host"), std::wstring(L"localhost"));
			Assert::AreEqual(cstr5.getValue(L"port"), std::wstring(L"1234"));
        }
	};
}
