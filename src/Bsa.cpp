#include "Bsa.h"
#include <iostream>

namespace Bsa {
//------------------------------------------------------------------------------------------------------
bool operator==(const Hash& a, const Hash& b) {
    return a.hash1 == b.hash1 && a.hash2 == b.hash2;
}
//------------------------------------------------------------------------------------------------------
bool operator<(const Hash& lhs, const Hash& rhs) {
    return ( lhs.hash1 < rhs.hash1 || !(rhs.hash1 < lhs.hash1) && lhs.hash2 < rhs.hash2 );
    /*
    Above code basically expands to

    if ( lhs.hash1 < rhs.hash1 )
        return true
    else if ( rhs.hash1 == lhs.hash1 )
        if ( lhs.hash2 < rhs.hash2 )
            return true
    return false
    */
}
//------------------------------------------------------------------------------------------------------
const size_t FileId::NOT_FOUND = 0xFFFFFFFF;
//------------------------------------------------------------------------------------------------------
BsaFile::BsaFile(const std::string& fileName) : mFile(fileName.c_str(), std::ios::binary) {
    if ( !mFile.is_open() || !mFile.good() ) {
        throw new BsaException("The BSA file could not be opened or was not in a state to be read");
    }

    mFile.read((char*)&mHeader, sizeof(Header));
    if ( mHeader.version != 0x00000100 ) { //all tes3 bsa files start with 00 00 01 00
        throw new BsaException("The BSA didn't start with the correct number. The file may not be a BSA file");
    }
    assert(mHeader.numberOfFiles>=0);

    mOffsets.resize(mHeader.numberOfFiles);
    mFile.read(reinterpret_cast<char*>(&mOffsets[0]), (sizeof (FileOffset))*mHeader.numberOfFiles);
    if ( mFile.eof() || mFile.fail() || mFile.gcount() != static_cast<int>((sizeof (FileOffset))*mHeader.numberOfFiles) ){
        throw new BsaException("Failed to read the file offsets");
    }

    //skip the file names, so we need to seekg to the hash table
    mFile.seekg(mHeader.hashTableOffset+12, std::ios_base::beg);
    if ( mFile.tellg() != mHeader.hashTableOffset+12 ){
        throw new BsaException("Failed to seekg to the hash offsets. The file may be corrupt");
    }

    mHashes.resize(mHeader.numberOfFiles);
    mFile.read(reinterpret_cast<char*>(&mHashes[0]), (sizeof (Hash))*mHeader.numberOfFiles);
    if ( mFile.eof() || mFile.fail() || mFile.gcount() != static_cast<int>((sizeof (Hash))*mHeader.numberOfFiles) ){
        throw new BsaException("Failed to read the file hashes");
    }

    //data directly follows the hashes
    mDataOffset = mFile.tellg();
}
//------------------------------------------------------------------------------------------------------
FileId BsaFile::getFileId(const std::string& name) {
    Hash h = getStringHash(name);
    std::vector<Hash>::iterator itr = binary_find(mHashes.begin(), mHashes.end(), h);
    return itr == mHashes.end() ? FileId(this, FileId::NOT_FOUND) : FileId(this, itr - mHashes.begin());
}
//------------------------------------------------------------------------------------------------------
void BsaFile::getFile(FileId id, char* dest) {
    assert(id._getOwner()==this);
    assert(id._getId()>=0);
    assert(id._getId()<mOffsets.size());

    size_t offset   = getFileOffset(id);
    size_t size     = getFileSize(id);

    mFile.seekg(offset, std::ios_base::beg);
    mFile.read(dest, size);

    if ( mFile.eof() ){
        throw new BsaException("The end of the file was reached when trying to read the data");
    }else if ( mFile.fail() ){
        throw new BsaException("The data file couldn't be read");
    }
}
//------------------------------------------------------------------------------------------------------
size_t BsaFile::getFileSize(FileId id) {
    assert(id._getOwner()==this);
    assert(id._getId()>=0);
    assert(id._getId()<mOffsets.size());

    return mOffsets[id._getId()].fileSize;
}
//------------------------------------------------------------------------------------------------------
size_t BsaFile::getFileOffset(FileId id) {
    assert(id._getOwner()==this);
    assert(id._getId()>=0);
    assert(id._getId()<mOffsets.size());

    return mOffsets[id._getId()].fileOffset + mDataOffset;
}
//------------------------------------------------------------------------------------------------------
Hash BsaFile::getStringHash(std::string str) {
    //string must be lower case and use / rather than \\.
    std::transform(str.begin(), str.end(), str.begin(), tolower);
    boost::replace_all(str, "/", "\\");

    Hash result;
    unsigned int len    = static_cast<unsigned int>(str.length());
    unsigned int l      = (len>>1);
    unsigned int sum, off, temp, i, n;

    for(sum = off = i = 0; i < l; i++) {
        sum ^= (static_cast<unsigned int>(str[i]))<<(off&0x1F);
        off += 8;
    }
    result.hash1 = sum;

    for(sum = off = 0; i < len; i++) {
        temp = (static_cast<unsigned int>(str[i]))<<(off&0x1F);
        sum ^= temp;
        n = temp & 0x1F;
        sum = (sum << (32-n)) | (sum >> n);
        off += 8;
    }
    result.hash2 = sum;

    return result;
}
//------------------------------------------------------------------------------------------------------

}
