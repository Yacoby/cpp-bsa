#define BOOST_TEST_DYN_LINK
#include "Bsa.h"
#define BOOST_TEST_MODULE BsaTest
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE( HashTest ){
    using namespace Bsa;
    Hash h1;
    h1.hash1 = 1;
    h1.hash2 = 2;

    Hash h2(h1);
    BOOST_CHECK( h1 == h2 );

    BOOST_CHECK( !(h1 < h2) );
    BOOST_CHECK( !(h2 < h1) );

    h2.hash2 = 0;
    BOOST_CHECK( !(h1 == h2) );
    BOOST_CHECK( h2 < h1 );

    h1.hash1 = 0;
    BOOST_CHECK( h1 < h2 );
}


BOOST_AUTO_TEST_CASE( LoadTest ){
    using namespace Bsa;

    BsaFile b("Data/Tribunal.bsa");

    //basic check
    BOOST_CHECK(b.getFileId("meshes/i/act_sotha_powertubes.nif").exists());
    //case shouldn't matter
    BOOST_CHECK(b.getFileId("meshes/i/act_sotha_poWertubeS.nif").exists());

    //neither should the slashes used
    BOOST_CHECK(b.getFileId("meshes\\i\\act_sotha_powertubes.nif").exists());

    //but it not exissting should
    BOOST_CHECK(!b.getFileId("textures/THIS_IS_NOT_A_TEXTUEEEEE").exists());
}

BOOST_AUTO_TEST_CASE( SizeTest ){
    using namespace Bsa;
    BsaFile b("Data/Tribunal.bsa");

    FileId id = b.getFileId("meshes/i/act_sotha_powertubes.nif");
    BOOST_REQUIRE(id.exists());

    BOOST_CHECK(b.getFileSize(id) == 86473);

    id = b.getFileId("textures/amel_summon_spark.dds");
    BOOST_REQUIRE(id.exists());
    BOOST_CHECK(b.getFileSize(id) == 4224);

}

BOOST_AUTO_TEST_CASE( OffsetTests ){
    using namespace Bsa;
    BsaFile b("Data/Tribunal.bsa");

    FileId id = b.getFileId("meshes/i/act_sotha_powertubes.nif");
    BOOST_REQUIRE(id.exists());
    BOOST_CHECK(b.getFileOffset(id) == 4349620);

    id = b.getFileId("textures/amel_summon_spark.dds");
    BOOST_REQUIRE(id.exists());
    BOOST_CHECK(b.getFileOffset(id) == 44175464);
}

BOOST_AUTO_TEST_CASE( LoadPowertubes ){
    using namespace Bsa;
    BsaFile b("Data/Tribunal.bsa");

    FileId id = b.getFileId("meshes/i/act_sotha_powertubes.nif");
    BOOST_REQUIRE(id.exists());

    size_t bsaSize = b.getFileSize(id);


    std::ifstream ifs("TestData/act_sotha_powertubes.nif", std::ios::binary);
    BOOST_REQUIRE(ifs.is_open());
    ifs.seekg(0, std::ios::end);
    size_t fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    //must agree on sizes
    BOOST_REQUIRE(fileSize == bsaSize);


    char* fileData = new char[fileSize];
    ifs.read(fileData, fileSize);

    char* bsaData = new char[bsaSize];
    b.getFile(id, bsaData);

    //both files should be the same
    BOOST_CHECK( memcmp(bsaData, fileData, bsaSize) == 0 );

    delete [] bsaData;
    delete [] fileData;
}
