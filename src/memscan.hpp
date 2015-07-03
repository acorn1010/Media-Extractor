// Coded by Paul 'Flyte'
// Provided as is, blah blah blah, give credit where credit is due, blah blah blah.

// Work in progress. Largely untested.

#ifndef MEMORY_HPP // Header guards. One of the few legit uses of the C preprocessor (lol)
#define MEMORY_HPP

#include <list>
#include <string>

#include <windows.h>

class Memory;

class Address {
	friend class Memory;
	friend class Scanner;

	Memory & memory_;
	unsigned long const addr_;

	Address(Memory & memory, unsigned long addr) :
	  memory_(memory), addr_(addr) {}

public:

    template <typename T>
	Address & operator=(T const & rhs) {
		memory_.WriteAddress(addr_, rhs);
		return *this;
	}

	template <typename T>
	operator T() const {
		return memory_.ReadAddress<T>(addr_);
	}

	unsigned long GetValue() const {
		return addr_;
	}
};

struct RegionInfo {
	bool IsReadable;
	bool IsWritable;
	unsigned long BaseAddress;
	unsigned long Size;

	RegionInfo() :
	  IsReadable(false), IsWritable(false),
		  BaseAddress(0), Size(0) {}

	RegionInfo(bool isReadable, bool isWritable, unsigned long baseAddress, unsigned long size) :
	  IsReadable(isReadable), IsWritable(isWritable),
		  BaseAddress(baseAddress), Size(size) {}


	template<typename T>
	bool IsInRegion(Address const & addr) const {
		if(Size < sizeof(T)) {
			return false;
		}
		return BaseAddress <= addr.GetValue() && addr.GetValue() < BaseAddress + Size - sizeof(T);
	}
};

class Memory {
	friend class Scanner;
	friend class Address;

protected:

	virtual bool WriteAddressInternal(void * addr, void const * value, unsigned int size) {
		::memcpy(addr, value, size);
		return true;
	}

	virtual bool ReadAddressInternal(void const * addr, void * value, unsigned int size) {
		::memcpy(value, addr, size);
		return true;
	}

	template <typename T>
	void WriteAddress(unsigned long addr, T const & value) {
		WriteAddressInternal((void *)addr, (const void *)&value, sizeof(T));
	}

	template <typename T>
	T ReadAddress(unsigned long addr) {
		T ret;
		ReadAddressInternal((const void *)addr, (void*)&ret, sizeof(T));
		return ret;
	}

public:

	virtual bool QueryMemory(unsigned long addr, RegionInfo & regionInfo) {
		::MEMORY_BASIC_INFORMATION mbi = { 0 };

		if(::VirtualQuery((void*)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
		{
			bool isReadable = false;
			bool isWriteable = false;

			if(mbi.State & (MEM_COMMIT) )
			{
				if(mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
					PAGE_EXECUTE_WRITECOPY | PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)) {
						isReadable = true;
				}
				if(mbi.Protect & (PAGE_EXECUTE_READWRITE |
					PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY)) {
						isWriteable = true;
				}
			}

			regionInfo = RegionInfo(isReadable, isWriteable, (unsigned long)mbi.BaseAddress, mbi.RegionSize);
			return true;
		}

		return false;
	}

	Address Reference(unsigned long addr) {
		return Address(*this, addr);
	}
};

class RemoteMemory : public Memory {
	const ::HANDLE process_;

protected:

	virtual bool WriteAddressInternal(void * addr, void const * value, unsigned int size) {
		::SIZE_T junk;
		return WriteProcessMemory( process_, addr, value, size, &junk) ? true : false;
	}

	virtual bool ReadAddressInternal(void const * addr, void * value, unsigned int size) {
		::SIZE_T junk;
		return ::ReadProcessMemory(process_, addr, value, size, &junk) ? true : false;
	}

public:

	virtual bool QueryMemory(unsigned long addr, RegionInfo & regionInfo) {
		::MEMORY_BASIC_INFORMATION mbi = { 0 };

		if(::VirtualQueryEx(process_, (void*)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
		{
			bool isReadable = false;
			bool isWriteable = false;

			if(mbi.State & (MEM_COMMIT) )
			{
				if(mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
					PAGE_EXECUTE_WRITECOPY | PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)) {
						isReadable = true;
				}
				if(mbi.Protect & (PAGE_EXECUTE_READWRITE |
					PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY)) {
						isWriteable = true;
				}
			}
            
			regionInfo = RegionInfo(isReadable, isWriteable, (unsigned long)mbi.BaseAddress, mbi.RegionSize);
			return true;
		}

		return false;
	}

	RemoteMemory(::HANDLE process)
		: process_(process) {}
};

class Scanner {
	Memory & memory_;
	static Memory localMemory_;

    std::string mask_; // last mask used by AoB scan

    /************************************************************************/
    /* Scans a region in memory.  Addresses are incremented by 1 each time. */
    /************************************************************************/
	template <typename T, typename S>
	void ScanInternal(T const & value, RegionInfo & regInfo, std::list<Address> & items, S & comparer) {
		for(unsigned long x = regInfo.BaseAddress; x < regInfo.BaseAddress + regInfo.Size - sizeof(T); ++x) {
			T val;
			if(memory_.ReadAddressInternal((const void *)x, (void *)&val, sizeof(T))) {
				if(comparer(value, val)) {
					items.push_back(Address(memory_, x));
				}
			} else { // if we're not able to read from the address, then break
				break;
			}
		}
	}

    template <typename T, typename S>
    void ScanAoBInternal(T const & value, RegionInfo & regInfo, std::list<Address> & items, S & comparer, const unsigned long valueLength) {
        for(unsigned long x = regInfo.BaseAddress; x < regInfo.BaseAddress + regInfo.Size - valueLength; ++x) {
            if(comparer(value, (const char *)x, valueLength, mask_)) {
                items.push_back(Address(memory_, x));
            }
        }
    }

	template <typename T, typename S>
	void ScanEnterTry(T const & value, RegionInfo & regInfo, std::list<Address> & items, S & comparer) {
		__try {
			ScanInternal(value, regInfo, items, comparer);
		} __except(true) {
			// Do nothing.
		}
	}

    template <typename T, typename S>
    void ScanAoBEnterTry(T const & value, RegionInfo & regInfo, std::list<Address> & items, S & comparer, const unsigned long valueLength) {
        __try {
            ScanAoBInternal(value, regInfo, items, comparer,valueLength);
        } __except(true) {
            // Do nothing.
        }
    }

	template<typename T, typename S>
	int ScanNextInternal(T const & value, std::list<Address> & items, std::list<Address>::iterator & iter, RegionInfo & regInfo, S & comparer) {
		while(iter != items.end()) {
			if(!regInfo.IsInRegion<T>(*iter)) {
				if(!memory_.QueryMemory(iter->GetValue(), regInfo)) {
					while(regInfo.IsInRegion<T>(*iter)) {
						iter = items.erase(iter);
						if(iter == items.end()) {
							break;
						}
					}
					continue;
				}
				if(!regInfo.IsReadable) {
					while(regInfo.IsInRegion<T>(*iter)) {
						iter = items.erase(iter);
						if(iter == items.end()) {
							break;
						}
					}
					continue;
				}
			}

			if(!comparer(value, (T)(*iter))) {
				iter = items.erase(iter);
			} else {
				++iter;
			}
		}
		return items.size();
	}

	void IFuckingHateSEH(std::list<Address> & items, std::list<Address>::iterator & iter) {
		iter = items.erase(iter);
	}

	template<typename T, typename S>
	int ScanNextEnterTry(T const & value, std::list<Address> & items, std::list<Address>::iterator & iter, RegionInfo & regInfo, S & comparer) {
		int i = 0;
		for(;;) {
			__try {
				i = ScanNextInternal(value, items, iter, regInfo, comparer);
				break;
			} __except(true) {
				IFuckingHateSEH(items, iter);
			}
		}
		return i;
	}

	template<typename T>
	static bool Equal(T a, T b) {
		return a == b;
	}

    static bool EqualAoB( const char *a, const char *b, const unsigned long valueLength, const std::string &mask )
    {
        bool result = true;
        
        for( unsigned long i=0; i < valueLength; ++i )
        {
            // if a and b aren't equal and b isn't a wildcard
            if( a[i] != b[i] && mask[i] != '?' )
            {
                result = false;
                break;
            }
        }

        return result;
    }

    template <typename T, class S>
    int ScanAoB(T const & value, 
        std::list<Address> & items, 
        S & comparer, 
        unsigned int maxItemsToScan = 1000,
        unsigned long valueLength = 0, // needed for AoBs else this MUST be 0
        unsigned long minimumApplicationAddress = 0, 
        unsigned long maximumApplicationAddress = 0) 
    {
        ::SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        if( valueLength == 0 )
            valueLength = sizeof(T);
        if( minimumApplicationAddress == 0 )
            minimumApplicationAddress = (unsigned long)sysInfo.lpMinimumApplicationAddress;
        if( maximumApplicationAddress == 0 )
            maximumApplicationAddress = (unsigned long)sysInfo.lpMaximumApplicationAddress;

        for( unsigned long i = minimumApplicationAddress;
            i < maximumApplicationAddress - sizeof(T);) 
        {
            RegionInfo regInfo;
            if(memory_.QueryMemory(i, regInfo)) 
            {
                if(regInfo.IsReadable) 
                { // only scan if the memory can be read
                    ScanAoBEnterTry(value, regInfo, items, comparer, valueLength);
                }
                i = regInfo.BaseAddress + regInfo.Size;
                if( items.size() >= maxItemsToScan ) // prevent bad_alloc by limiting size
                    return items.size();
            } 
            else 
            {
                return 0; // we failed to query the memory (ONOES!)
            }
        }

        return items.size();
    }

public:

	Scanner()
		: memory_(localMemory_) {}

	Scanner(Memory & memory)
		: memory_(memory) {}


    // Get region associated with an address
    RegionInfo GetRegion( unsigned long addr )
    {
        RegionInfo regInfo;
        memory_.QueryMemory(addr, regInfo);

        return regInfo;
    }

	template <typename T>
	int Scan(T const & value, std::list<Address> & items) {
		return Scan(value, items, Equal<T>, 1000);
	}

	template <typename T, class S>
	int Scan(T const & value, 
        std::list<Address> & items, 
        S & comparer, 
        unsigned int maxItemsToScan = 1000,
        unsigned long minimumApplicationAddress = 0, 
        unsigned long maximumApplicationAddress = 0) 
    {
		::SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
        
        if( minimumApplicationAddress == 0 )
            minimumApplicationAddress = (unsigned long)sysInfo.lpMinimumApplicationAddress;
        if( maximumApplicationAddress == 0 )
            maximumApplicationAddress = (unsigned long)sysInfo.lpMaximumApplicationAddress;

		for( unsigned long i = minimumApplicationAddress;
			i < maximumApplicationAddress - sizeof(T);) 
        {
			RegionInfo regInfo;
			if(memory_.QueryMemory(i, regInfo)) 
            {
				if(regInfo.IsReadable) 
                { // only scan if the memory can be read
					ScanEnterTry(value, regInfo, items, comparer);
				}
				i = regInfo.BaseAddress + regInfo.Size;
                if( items.size() >= maxItemsToScan ) // prevent bad_alloc by limiting size
                    return items.size();
			} 
            else 
            {
				return 0; // we failed to query the memory (ONOES!)
			}
		}

		return items.size();
	}

	template <typename T>
	int ScanNext(T const & value, std::list<Address> & items) {
		return ScanNext(value, items, Equal<T>);
	}

	template <typename T, class S>
	int ScanNext(T const & value, std::list<Address> & items, S & comparer) {
		std::list<Address>::iterator iter = items.begin();
		RegionInfo regInfo;

		return ScanNextEnterTry(value, items, iter, regInfo, comparer);
	}

    // Given an AoB, optional offset, and whether or not it's a call, returns an address
    // You may also specify a minimum and maximum address to scan for the AoB to greatly 
    // increase scan time.
    // This is a convenience function.
    unsigned long GetAddressFromAoB( const char *aob,
        const std::string &mask, // determines size of string and the mask
        const unsigned int index=1, 
        unsigned long minimumApplicationAddress = 0, 
        unsigned long maximumApplicationAddress = 0
        )
    {
        unsigned long result = 0;
        std::list<Address> addressList;
        ::SYSTEM_INFO sysInfo;

        GetSystemInfo(&sysInfo);
        if( minimumApplicationAddress == 0 )
            minimumApplicationAddress = (unsigned long)sysInfo.lpMinimumApplicationAddress;
        if( maximumApplicationAddress == 0 )
            maximumApplicationAddress = (unsigned long)sysInfo.lpMaximumApplicationAddress;

        mask_ = mask;
        ScanAoB( aob, addressList, EqualAoB, index, mask.size(), minimumApplicationAddress, maximumApplicationAddress );

        std::list<Address>::iterator iter = addressList.begin();
        for( unsigned int i=1; i < index && iter != addressList.end(); ++iter, ++i );

        if( iter != addressList.end() && 
            iter->addr_ <= maximumApplicationAddress && 
            iter->addr_ >= minimumApplicationAddress )
            result = iter->addr_;

        return result;
    }

    // Given an AoB and optional offsets, returns an address list
    // You may also specify a minimum and maximum address to scan for the AoB to greatly 
    // increase scan time.
    std::list<unsigned long> GetListFromAoB( const char *aob,
        const std::string &mask, // determines size of string and the mask
        unsigned long minimumApplicationAddress = 0, 
        unsigned long maximumApplicationAddress = 0
        )
    {
        std::list<unsigned long> result;
        std::list<Address> addressList;
        ::SYSTEM_INFO sysInfo;

        GetSystemInfo( &sysInfo );
        if( minimumApplicationAddress == 0 )
            minimumApplicationAddress = (unsigned long)sysInfo.lpMinimumApplicationAddress;
        if( maximumApplicationAddress == 0 )
            maximumApplicationAddress = (unsigned long)sysInfo.lpMaximumApplicationAddress;

        mask_ = mask;
        ScanAoB( aob, addressList, EqualAoB, 1000, mask.size(), minimumApplicationAddress, maximumApplicationAddress );

        std::list<Address>::iterator iter = addressList.begin();
        for( ; iter != addressList.end(); ++iter )
        {
            if( iter->addr_ <= maximumApplicationAddress &&
                iter->addr_ >= minimumApplicationAddress )
                result.push_back( iter->addr_ );
        }

        return result;
    }
};

#endif
