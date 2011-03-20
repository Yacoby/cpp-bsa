#ifndef __BSA_H_
#define __BSA_H_

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

/*
 * I used several boost classes/fucntions as it is easier and it doesn't make sense not to take advantage of them
 *
 * Both of the uses are easy to replace with something else if you don't have boost or don't want the added dependacy
 */

//used for replacing / with \\ in the file strings
#include <boost/algorithm/string/replace.hpp>

//used for ensuring that the BSA file can't be copied. Writing some copy constructors would work.
#include <boost/utility.hpp>

namespace Bsa {
//------------------------------------------------------------------------------------------------------
class BsaFile;
class FileId;
//------------------------------------------------------------------------------------------------------
/**
 * @todo add in line/file information as well as overload for ostream << operator.
 */
class BsaException : public std::exception{
public:
    BsaException(const std::string& message) : mMessage(message){
    }
    ~BsaException() throw(){}

    const std::string& getMessage(){
        return mMessage;
    }
private:
    const std::string& mMessage;
};
//------------------------------------------------------------------------------------------------------
/**
 * Bsa file header
 */
struct Header {
    int version;
    int hashTableOffset;
    int numberOfFiles;
};
//------------------------------------------------------------------------------------------------------
struct FileOffset {
    int fileSize;
    /**
     * Offset to the start of the file from the start of the bsa data section
     */
    int fileOffset;
};
//------------------------------------------------------------------------------------------------------
struct Hash {
    unsigned int hash1;
    unsigned int hash2;
};
bool operator==(const Hash& a, const Hash& b);
/**
 * Compares the two hashes, with priority going to hash1
 */
bool operator<(const Hash& lhs, const Hash& rhs);
//------------------------------------------------------------------------------------------------------
/**
 * This is a wrapper arround an integer to abstract the interface from the unerlying representation allowing
 * the easy change to another system that isn't index based.
 *
 * The overhead of using this should hopefully be
 * minimal as it is basically two integers. If it was a concern one of the integers (the pointer to the created
 * bsa file) could be removed at a slight loss to the security of the code.
 *
 * I have a feeling that the cost is so minimal compared to the disk IO it isn't worth even thinking about.
 */
class FileId {
public:
    /**
     * @return true if the file exists
     */
    inline bool exists() {
        return mId != NOT_FOUND;
    }
    /**
     * This function shouldn't be used by anything but the BsaFile class. It is used to check that the
     * file id belongs to the correct Bsa file. It is possile to pass a fileId constructed from another bsa file
     *
     * The check is only done with asserts, so don't rely on it in release mode.
     *
     * @return the BsaFile that created the object
     */
    inline BsaFile* _getOwner() {
        return mFileOwner;
    }
    /**
     * Should only be used by the BsaFile class as there is no guarentee that it will not change.
     *
     * @return the id of the file
     */
    inline size_t _getId() {
        return mId;
    }
private:
    friend class BsaFile;
    /**
     * Private constructor. This ensures that the class can only be constructed by the Bsa class.
     *
     * Users of the api shouldn't be allowed to construct this as the constructor exposeses some of the underlying
     * representation
     *
     * @param fileOwner this can be used to check the the FileId is from the right bsa
     * @param id the id of the file (the array offset)
     */
    FileId(BsaFile* fileOwner, size_t id) {
        mFileOwner = fileOwner;
        mId = id;
    }

    /**
     * The internal representation of a file that doens't exist (all bits set to 1)
     */
    const static size_t NOT_FOUND;

    BsaFile* mFileOwner;
    size_t mId;
};
//------------------------------------------------------------------------------------------------------
/**
 * This represents an open bsa file. To remove added complexity the state isn't anything but open.
 *
 * It cannot be coppies as it would not be clear as to which class then owned the file. Totally forbidding it removes
 * that decision. I cannot also see why anyone would want to do anything expensive like a copy.
 */
class BsaFile : boost::noncopyable{
public:
    /**
     * The constructor loads the file. This is done here to avoid added complexity of having to deal with the
     * open sate of the file. It is always open until the class is destructed.
     *
     * The constructor will throw exceptions if the file cannot be opened for reading or if the file is not a
     * valid bsa file. This is checked via the magic number at the start of the file.
     *
     * @param fileName Path to the file. Can be relative
     */
    BsaFile(const std::string& fileName);

    /**
     * Gets the file Id for a given file name. Complexity is O(log n) as it is based on a binary search
     *
     * @param name the file name to search for. This ignores case and the direction of slashes.
     */
    FileId getFileId(const std::string& name);

    /**
     * Not very c++ like, but this function copies the file into the destination buffer.
     * There are no checks on the buffer done, so that it is vital that the buffer is the correct size
     *
     * This will throw if the file wasn't read correctly. If ios::exceptions is set, it will throw ios_base::failure,
     * otherwise it will throw a BsaException
     *
     * @param id the file id of the file to extract
     * @param dest the pointer to the start of a destination array containing enough space for the data
     */
    void getFile(FileId id, char* dest);

    /**
     * Required for working out the required size of the data array to create
     *
     * @param id the file id
     * @return the size of the file
     */
    size_t getFileSize(FileId id);

    /**
     * Gets the offset of the file within the BSA. This is not the relative offset but the offset from the start of the file
     *
     * Can't see it being much use for it being public other than debugging/unit tests. It is used when extracting data
     * from the file
     *
     * @return the offset of the file from the beinging of the file
     */
    size_t getFileOffset(FileId id);
private:
    Header mHeader;
    std::vector<FileOffset> mOffsets;
    std::vector<Hash> mHashes;

    std::ifstream mFile;

    /**
     * Offset of the data section of the Bsa file. This is required as the offset as stored in the
     * FileOffset struct is relative to the start of the data section
     */
    size_t mDataOffset;

    /**
     * Creates a hash from the givne string. It uses ghostwheels algorithm for creating the hashes
     *
     * This will convert all slashes into backslashes and ensure that everything is lower case.
     *
     * @param str the string to create the hash of. The string is not passed byref as it is altered
     *          by the function to ensure that it is lower case etc.
     *
     * @return the hash of the string
     */
    Hash getStringHash(std::string str);

    /**
     * Binary search algorithm as it should have been in the stl. If the iterator is random access the
     * complexity is at wosrst O(log n) otherwise it is at the worst case O(n)
     *
     * @return a iterator pointing to the match or the end if the item is not found
     */
    template<class ForwardIterator, class T>
    ForwardIterator binary_find(ForwardIterator begin, ForwardIterator end, const T& value) {
        ForwardIterator first = std::lower_bound(begin, end, value);
        return  (first != end && *first == value) ? first : end;
    }

};
//------------------------------------------------------------------------------------------------------

}
#endif //__BSA_H_
