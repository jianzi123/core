/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_SW_SOURCE_FILTER_WW8_WW8SCAN_HXX
#define INCLUDED_SW_SOURCE_FILTER_WW8_WW8SCAN_HXX

#ifndef LONG_MAX
#include <limits.h>
#endif
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <stack>
#include <unordered_map>
#include <vector>

#include <tools/solar.h>
#include <tools/stream.hxx>
#include <rtl/ustring.hxx>
#include "sortedarray.hxx"

#include "ww8struc.hxx"
#include <types.hxx>

#include <unomid.h>

//--Line below which the code has meaningful comments

//Commonly used string literals for stream and storage names in word docs
namespace SL
{
#   define DEFCONSTSTRINGARRAY(X) extern const char a##X[sizeof(#X)]
    DEFCONSTSTRINGARRAY(ObjectPool);
    DEFCONSTSTRINGARRAY(1Table);
    DEFCONSTSTRINGARRAY(0Table);
    DEFCONSTSTRINGARRAY(Data);
    DEFCONSTSTRINGARRAY(CheckBox);
    DEFCONSTSTRINGARRAY(ListBox);
    DEFCONSTSTRINGARRAY(TextBox);
    DEFCONSTSTRINGARRAY(TextField);
    DEFCONSTSTRINGARRAY(MSMacroCmds);
}

struct SprmInfo
{
    unsigned int nLen : 6;
    unsigned int nVari : 2;
};

struct SprmInfoRow {
    sal_uInt16 nId;         ///< A ww8 sprm is hardcoded as 16bits
    SprmInfo info;
};

class wwSprmSearcher {
public:
    wwSprmSearcher(SprmInfoRow const * rows, std::size_t size) {
        for (std::size_t i = 0; i != size; ++i) {
            bool ins = map_.insert(Map::value_type(rows[i].nId, rows[i].info))
                .second;
            assert(ins); (void) ins;
        }
    }

    SprmInfo const * search(sal_uInt16 id) const {
        Map::const_iterator i(map_.find(id));
        return i == map_.end() ? 0 : &i->second;
    }

private:
    typedef std::unordered_map<sal_uInt16, SprmInfo> Map;

    Map map_;
};

/**
    wwSprmParser knows how to take a sequence of bytes and split it up into
    sprms and their arguments
*/
class wwSprmParser
{
private:
    ww::WordVersion meVersion;
    sal_uInt8 mnDelta;
    const wwSprmSearcher *mpKnownSprms;
    static const wwSprmSearcher* GetWW8SprmSearcher();
    static const wwSprmSearcher* GetWW6SprmSearcher();
    static const wwSprmSearcher* GetWW2SprmSearcher();

    SprmInfo GetSprmInfo(sal_uInt16 nId) const;

    sal_uInt8 SprmDataOfs(sal_uInt16 nId) const;

    enum SprmType {L_FIX=0, L_VAR=1, L_VAR2=2};

public:
    //7- ids are very different to 8+ ones
    explicit wwSprmParser(ww::WordVersion eVersion);
    /// Return the SPRM id at the beginning of this byte sequence
    sal_uInt16 GetSprmId(const sal_uInt8* pSp) const;

    sal_uInt16 GetSprmSize(sal_uInt16 nId, const sal_uInt8* pSprm) const;

    /// Get known len of a sprms head, the bytes of the sprm id + any bytes
    /// reserved to hold a variable length
    sal_uInt16 DistanceToData(sal_uInt16 nId) const;

    /// Get len of a sprms data area, ignoring the bytes of the sprm id and
    /// ignoring any len bytes. Reports the remaining data after those bytes
    sal_uInt16 GetSprmTailLen(sal_uInt16 nId, const sal_uInt8 * pSprm) const;

    /// The minimum acceptable sprm len possible for this type of parser
    int MinSprmLen() const { return (IsSevenMinus(meVersion)) ? 2 : 3; }

    /// Returns the offset to data of the first sprm of id nId, 0
    //  if not found. nLen must be the <= length of pSprms
    sal_uInt8* findSprmData(sal_uInt16 nId, sal_uInt8* pSprms, sal_uInt16 nLen)
        const;
};

//Read a Pascal-style, i.e. single byte string length followed
//by string contents
inline OUString read_uInt8_PascalString(SvStream& rStrm, rtl_TextEncoding eEnc)
{
    return read_uInt8_lenPrefixed_uInt8s_ToOUString(rStrm, eEnc);
}

inline OUString read_uInt16_PascalString(SvStream& rStrm)
{
    return read_uInt16_lenPrefixed_uInt16s_ToOUString(rStrm);
}

//Belt and Braces strings, i.e. Pascal-style strings followed by
//null termination, Spolsky calls them "fucked strings" FWIW
//http://www.joelonsoftware.com/articles/fog0000000319.html
OUString read_uInt8_BeltAndBracesString(SvStream& rStrm, rtl_TextEncoding eEnc);
OUString read_uInt16_BeltAndBracesString(SvStream& rStrm);

//--Line above which the code has meaningful comments

class  WW8Fib;
class  WW8ScannerBase;
class  WW8PLCFspecial;
struct WW8PLCFxDesc;
class  WW8PLCFx_PCD;

/**
 reads array of strings (see MS documentation: STring TaBle stored in File)
 returns NOT the original pascal strings but an array of converted char*

 attention: the *extra data* of each string are SKIPPED and ignored
 */
void WW8ReadSTTBF(bool bVer8, SvStream& rStrm, sal_uInt32 nStart, sal_Int32 nLen,
    sal_uInt16 nExtraLen, rtl_TextEncoding eCS, ::std::vector<OUString> &rArray,
    ::std::vector<ww::bytes>* pExtraArray = 0, ::std::vector<OUString>* pValueArray = 0);

struct WW8FieldDesc
{
    long nLen;              ///< total length (to skip over text)
    WW8_CP nSCode;          ///< start of instructions code
    WW8_CP nLCode;          ///< length
    WW8_CP nSRes;           ///< start of result
    WW8_CP nLRes;           ///< length ( == 0, if no result )
    sal_uInt16 nId;             ///< WW-id for fields
    sal_uInt8 nOpt;              ///< WW-Flags ( e.g.: changed by user )
    bool bCodeNest:1;       ///< instruction used recursively
    bool bResNest:1;        ///< instruction inserted into result
};

struct WW8PLCFxSave1
{
    sal_uLong nPLCFxPos;
    sal_uLong nPLCFxPos2;       ///< for PLCF_Cp_Fkp: PieceIter-Pos
    long nPLCFxMemOfs;
    WW8_CP nStartCp;        ///< for cp based iterator like PAP and CHP
    long nCpOfs;
    WW8_FC nStartFC;
    WW8_CP nAttrStart;
    WW8_CP nAttrEnd;
    bool bLineEnd;
};

/**
    among others for fields, that is, the same number of attr as positions,
    if Ctor-Param bNoEnd = false
*/
class WW8PLCFspecial        // iterator for PLCFs
{
private:
    sal_Int32* pPLCF_PosArray;  ///< pointer to Pos-array and to the whole structure
    sal_uInt8*  pPLCF_Contents;  ///< pointer to content-array-part of Pos-array
    long nIMax;             ///< number of elements
    long nIdx;              ///< marker where we currently are
    sal_uInt32 nStru;

public:
    WW8PLCFspecial(SvStream* pSt, sal_uInt32 nFilePos, sal_uInt32 nPLCF,
        sal_uInt32 nStruct);
    ~WW8PLCFspecial() { delete[] pPLCF_PosArray; }
    long GetIdx() const { return nIdx; }
    void SetIdx( long nI ) { nIdx = nI; }
    long GetIMax() const { return nIMax; }
    bool SeekPos(long nPos);            // walks over FC- or CP-value
                                        // resp. next biggest value
    bool SeekPosExact(long nPos);
    sal_Int32 Where() const
        { return ( nIdx >= nIMax ) ? SAL_MAX_INT32 : pPLCF_PosArray[nIdx]; }
    bool Get(WW8_CP& rStart, void*& rpValue) const;
    bool GetData(long nIdx, WW8_CP& rPos, void*& rpValue) const;

    const void* GetData( long nInIdx ) const
    {
        return ( nInIdx >= nIMax ) ? 0
            : static_cast<const void*>(&pPLCF_Contents[nInIdx * nStru]);
    }
    sal_Int32 GetPos( long nInIdx ) const
        { return ( nInIdx >= nIMax ) ? SAL_MAX_INT32 : pPLCF_PosArray[nInIdx]; }

    void advance()
    {
        if (nIdx <= nIMax)
            ++nIdx;
    }
};

/** simple Iterator for SPRMs */
class WW8SprmIter
{
private:
    const wwSprmParser &mrSprmParser;
    // these members will be updated
    const sal_uInt8* pSprms; // remaining part of the SPRMs ( == start of akt. SPRM)
    const sal_uInt8* pAktParams; // start of akt. SPRM's parameters
    sal_uInt16 nAktId;
    sal_uInt16 nAktSize;

    long nRemLen;   // length of remaining SPRMs (including akt. SPRM)

    void UpdateMyMembers();

public:
    explicit WW8SprmIter( const sal_uInt8* pSprms_, long nLen_,
        const wwSprmParser &rSprmParser);
    void  SetSprms( const sal_uInt8* pSprms_, long nLen_ );
    const sal_uInt8* FindSprm(sal_uInt16 nId);
    void  advance();
    const sal_uInt8* GetSprms() const
        { return ( pSprms && (0 < nRemLen) ) ? pSprms : 0; }
    const sal_uInt8* GetAktParams() const { return pAktParams; }
    sal_uInt16 GetAktId() const { return nAktId; }

private:
    WW8SprmIter(const WW8SprmIter&) SAL_DELETED_FUNCTION;
    WW8SprmIter& operator=(const WW8SprmIter&) SAL_DELETED_FUNCTION;
};

/* among others for FKPs to normal attr., i.e. one less attr than positions */
class WW8PLCF                       // Iterator for PLCFs
{
private:
    WW8_CP* pPLCF_PosArray; // pointer to Pos-array and the whole structure
    sal_uInt8* pPLCF_Contents;   // pointer to content-array-part of Pos-array
    sal_Int32 nIMax;            // number of elements
    sal_Int32 nIdx;
    int nStru;

    void ReadPLCF(SvStream& rSt, WW8_FC nFilePos, sal_uInt32 nPLCF);

    /*
        If a PLC is missing in the doc and the FKPs stand alone,
        we create a PLC with this:
    */
    void GeneratePLCF(SvStream& rSt, sal_Int32 nPN, sal_Int32 ncpN);

    void MakeFailedPLCF();

public:
    WW8PLCF(SvStream& rSt, WW8_FC nFilePos, sal_Int32 nPLCF, int nStruct,
        WW8_CP nStartPos = -1);

    /*
        the following ctor generates a PLC from nPN and ncpN, if necessary
    */
    WW8PLCF(SvStream& rSt, WW8_FC nFilePos, sal_Int32 nPLCF, int nStruct,
        WW8_CP nStartPos, sal_Int32 nPN, sal_Int32 ncpN);

    ~WW8PLCF(){ delete[] pPLCF_PosArray; }
    sal_Int32 GetIdx() const { return nIdx; }
    void SetIdx( sal_Int32 nI ) { nIdx = nI; }
    sal_Int32 GetIMax() const { return nIMax; }
    bool SeekPos(WW8_CP nPos);
    WW8_CP Where() const;
    bool Get(WW8_CP& rStart, WW8_CP& rEnd, void*& rpValue) const;
    void advance() { if( nIdx < nIMax ) ++nIdx; }

    const void* GetData( sal_Int32 nInIdx ) const
    {
        return ( nInIdx >= nIMax ) ? 0 :
            static_cast<const void*>(&pPLCF_Contents[nInIdx * nStru]);
    }
};

/* for Piece Table (.i.e. FastSave Table) */
class WW8PLCFpcd
{
    friend class WW8PLCFpcd_Iter;

    sal_Int32* pPLCF_PosArray;  // pointer to Pos-array and the whole structure
    sal_uInt8*  pPLCF_Contents;  // pointer to content-array-part of Pos-array
    long nIMax;
    sal_uInt32 nStru;

public:
    WW8PLCFpcd(SvStream* pSt, sal_uInt32 nFilePos, sal_uInt32 nPLCF,
        sal_uInt32 nStruct);
    ~WW8PLCFpcd(){ delete[] pPLCF_PosArray; }
};

/* multiple WW8PLCFpcd_Iter may point to the same WW8PLCFpcd !!!  */
class WW8PLCFpcd_Iter
{
private:
    WW8PLCFpcd& rPLCF;
    long nIdx;

    WW8PLCFpcd_Iter(const WW8PLCFpcd_Iter&) SAL_DELETED_FUNCTION;
    WW8PLCFpcd_Iter& operator=(const WW8PLCFpcd_Iter&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFpcd_Iter( WW8PLCFpcd& rPLCFpcd, long nStartPos = -1 );
    long GetIdx() const { return nIdx; }
    void SetIdx( long nI ) { nIdx = nI; }
    long GetIMax() const { return rPLCF.nIMax; }
    bool SeekPos(long nPos);
    sal_Int32 Where() const;
    bool Get(WW8_CP& rStart, WW8_CP& rEnd, void*& rpValue) const;
    void advance()
    {
        if( nIdx < rPLCF.nIMax )
            ++nIdx;
    }
};

// PLCF-type:
enum ePLCFT{ CHP=0, PAP, SEP, /*HED, FNR, ENR,*/ PLCF_END };

//Its hardcoded that eFTN be the first one: A very poor hack, needs to be fixed
enum eExtSprm { eFTN = 256, eEDN = 257, eFLD = 258, eBKN = 259, eAND = 260, eATNBKN = 261 };

/*
    pure virtual:
*/
class WW8PLCFx              // virtual iterator for Piece Table Exceptions
{
private:
    ww::WordVersion meVer;  // Version number of FIB
    bool bIsSprm;           // PLCF of Sprms or other stuff ( Footnote, ... )
    WW8_FC nStartFc;
    bool bDirty;

    WW8PLCFx(const WW8PLCFx&) SAL_DELETED_FUNCTION;
    WW8PLCFx& operator=(const WW8PLCFx&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx(ww::WordVersion eVersion, bool bSprm)
        : meVer(eVersion)
        , bIsSprm(bSprm)
        , nStartFc(-1)
        , bDirty(false)
    {
    }
    virtual ~WW8PLCFx() {}
    bool IsSprm() const { return bIsSprm; }
    virtual sal_uInt32 GetIdx() const = 0;
    virtual void SetIdx( sal_uLong nIdx ) = 0;
    virtual sal_uLong GetIdx2() const;
    virtual void SetIdx2( sal_uLong nIdx );
    virtual bool SeekPos(WW8_CP nCpPos) = 0;
    virtual WW8_FC Where() = 0;
    virtual void GetSprms( WW8PLCFxDesc* p );
    virtual long GetNoSprms( WW8_CP& rStart, WW8_CP&, sal_Int32& rLen );
    virtual void advance() = 0;
    virtual sal_uInt16 GetIstd() const { return 0xffff; }
    virtual void Save( WW8PLCFxSave1& rSave ) const;
    virtual void Restore( const WW8PLCFxSave1& rSave );
    ww::WordVersion GetFIBVersion() const { return meVer; }
    void SetStartFc( WW8_FC nFc ) { nStartFc = nFc; }
    WW8_FC GetStartFc() const { return nStartFc; }
    void SetDirty(bool bIn) {bDirty=bIn;}
    bool GetDirty() const {return bDirty;}
};

class WW8PLCFx_PCDAttrs : public WW8PLCFx
{
private:
    WW8PLCFpcd_Iter* pPcdI;
    WW8PLCFx_PCD* pPcd;
    sal_uInt8* const* pGrpprls; // attribute of Piece-table
    SVBT32 aShortSprm;          // mini storage: can contain ONE sprm with
                                // 1 byte param
    sal_uInt16 nGrpprls;            // attribute count of this

    WW8PLCFx_PCDAttrs(const WW8PLCFx_PCDAttrs&) SAL_DELETED_FUNCTION;
    WW8PLCFx_PCDAttrs& operator=(const WW8PLCFx_PCDAttrs&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_PCDAttrs(ww::WordVersion eVersion, WW8PLCFx_PCD* pPLCFx_PCD,
        const WW8ScannerBase* pBase );
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nI ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual void GetSprms( WW8PLCFxDesc* p ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;

    WW8PLCFpcd_Iter* GetIter() const { return pPcdI; }
};

class WW8PLCFx_PCD : public WW8PLCFx            // iterator for Piece table
{
private:
    WW8PLCFpcd_Iter* pPcdI;
    bool bVer67;
    WW8_CP nClipStart;

    WW8PLCFx_PCD(const WW8PLCFx_PCD&) SAL_DELETED_FUNCTION;
    WW8PLCFx_PCD& operator=(const WW8PLCFx_PCD&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_PCD(ww::WordVersion eVersion, WW8PLCFpcd* pPLCFpcd,
        WW8_CP nStartCp, bool bVer67P);
    virtual ~WW8PLCFx_PCD();
    sal_uLong GetIMax() const;
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nI ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual long GetNoSprms( WW8_CP& rStart, WW8_CP&, sal_Int32& rLen ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    WW8_CP AktPieceStartFc2Cp( WW8_FC nStartPos );
    WW8_FC AktPieceStartCp2Fc( WW8_CP nCp );
    static void AktPieceFc2Cp(WW8_CP& rStartPos, WW8_CP& rEndPos,
        const WW8ScannerBase *pSBase);
    WW8PLCFpcd_Iter* GetPLCFIter() { return pPcdI; }
    void SetClipStart(WW8_CP nIn) { nClipStart = nIn; }
    WW8_CP GetClipStart() { return nClipStart; }

    static sal_Int32 TransformPieceAddress(long nfc, bool& bIsUnicodeAddress)
    {
        bIsUnicodeAddress = 0 == (0x40000000 & nfc);
        return bIsUnicodeAddress ?  nfc : (nfc & 0x3fffFFFF) / 2;
    }
};

/**
 Iterator for Piece Table Exceptions of Fkps
 works only with FCs, not with CPs !  ( Low-Level )
*/
class WW8PLCFx_Fc_FKP : public WW8PLCFx
{
public:
    class WW8Fkp        // Iterator for Formatted Disk Page
    {
    private:
        class Entry
        {
        public:
            WW8_FC mnFC;

            sal_uInt8* mpData;
            sal_uInt16 mnLen;
            sal_uInt16 mnIStd; // only for Fkp.Papx (actually Style-Nr)
            bool mbMustDelete;

            explicit Entry(WW8_FC nFC) : mnFC(nFC), mpData(0), mnLen(0),
                mnIStd(0), mbMustDelete(false) {}
            Entry(const Entry &rEntry);
            ~Entry();
            bool operator<(const Entry& rEntry) const;
            Entry& operator=(const Entry& rEntry);
        };

        sal_uInt8 maRawData[512];
        std::vector<Entry> maEntries;

        long nItemSize;     // either 1 Byte or a complete BX

        // Offset in Stream where last read of 52 bytes took place
        long nFilePos;
        sal_uInt8 mnIdx;         // Pos marker
        ePLCFT ePLCF;
        sal_uInt8 mnIMax;         // number of entries

        wwSprmParser maSprmParser;

        //Fill in an Entry with sanity testing
        void FillEntry(Entry &rEntry, sal_Size nDataOffset, sal_uInt16 nLen);

    public:
        WW8Fkp (ww::WordVersion eVersion, SvStream* pFKPStrm,
            SvStream* pDataStrm, long _nFilePos, long nItemSiz, ePLCFT ePl,
            WW8_FC nStartFc = -1);
        void Reset(WW8_FC nPos);
        long GetFilePos() const { return nFilePos; }
        sal_uInt8 GetIdx() const { return mnIdx; }
        bool SetIdx(sal_uInt8 nI);
        bool SeekPos(WW8_FC nFc);
        WW8_FC Where() const
        {
            return (mnIdx < mnIMax) ? maEntries[mnIdx].mnFC : WW8_FC_MAX;
        }
        void advance()
        {
            if (mnIdx < mnIMax)
                ++mnIdx;
        }
        sal_uInt8* Get( WW8_FC& rStart, WW8_FC& rEnd, sal_Int32& rLen ) const;
        sal_uInt16 GetIstd() const { return maEntries[mnIdx].mnIStd; }

        /*
            returns a real pointer to the Sprm of type nId,
            if such a thing is in the Fkp.
        */
        sal_uInt8* GetLenAndIStdAndSprms(sal_Int32& rLen) const;

        /*
            calls GetLenAndIStdAndSprms()...
        */
        const sal_uInt8* HasSprm( sal_uInt16 nId );
        bool HasSprm(sal_uInt16 nId, std::vector<const sal_uInt8 *> &rResult);

        const wwSprmParser &GetSprmParser() const { return maSprmParser; }
    };

private:
    SvStream* pFKPStrm;         // input file
    SvStream* pDataStrm;        // input file
    WW8PLCF* pPLCF;
    WW8Fkp* pFkp;

    /*
        Keep a cache of eMaxCache entries of previously seen pFkps, which
        speeds up considerably table parsing and load save plcfs for what turn
        out to be small text frames, which frames generally are

        size      : cache hits
        cache all : 19168 pap, 48 chp
        == 100    : 19166 pap, 48 chp
        == 50     : 18918 pap, 48 chp
        == 10     : 18549 pap, 47 chp
        == 5      : 18515 pap, 47 chp
    */
    typedef std::list<WW8Fkp*>::iterator myiter;
    std::list<WW8Fkp*> maFkpCache;
    enum Limits {eMaxCache = 5};

    bool NewFkp();

    WW8PLCFx_Fc_FKP(const WW8PLCFx_Fc_FKP&) SAL_DELETED_FUNCTION;
    WW8PLCFx_Fc_FKP& operator=(const WW8PLCFx_Fc_FKP&) SAL_DELETED_FUNCTION;

protected:
    ePLCFT ePLCF;
    WW8PLCFx_PCDAttrs* pPCDAttrs;

public:
    WW8PLCFx_Fc_FKP( SvStream* pSt, SvStream* pTableSt, SvStream* pDataSt,
        const WW8Fib& rFib, ePLCFT ePl, WW8_FC nStartFcL );
    virtual ~WW8PLCFx_Fc_FKP();
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_FC nFcPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    sal_uInt8* GetSprmsAndPos( WW8_FC& rStart, WW8_FC& rEnd, sal_Int32& rLen );
    virtual void advance() SAL_OVERRIDE;
    virtual sal_uInt16 GetIstd() const SAL_OVERRIDE;
    void GetPCDSprms( WW8PLCFxDesc& rDesc );
    const sal_uInt8* HasSprm( sal_uInt16 nId );
    bool HasSprm(sal_uInt16 nId, std::vector<const sal_uInt8 *> &rResult);
    bool HasFkp() const { return (0 != pFkp); }
};

/// iterator for Piece Table Exceptions of Fkps works on CPs (high-level)
class WW8PLCFx_Cp_FKP : public WW8PLCFx_Fc_FKP
{
private:
    const WW8ScannerBase& rSBase;
    WW8PLCFx_PCD* pPcd;
    WW8PLCFpcd_Iter *pPieceIter;
    WW8_CP nAttrStart, nAttrEnd;
    bool bLineEnd : 1;
    bool bComplex : 1;

    WW8PLCFx_Cp_FKP(const WW8PLCFx_Cp_FKP&) SAL_DELETED_FUNCTION;
    WW8PLCFx_Cp_FKP& operator=(const WW8PLCFx_Cp_FKP&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_Cp_FKP( SvStream* pSt, SvStream* pTableSt, SvStream* pDataSt,
        const WW8ScannerBase& rBase,  ePLCFT ePl );
    virtual ~WW8PLCFx_Cp_FKP();
    void ResetAttrStartEnd();
    sal_uLong GetPCDIMax() const;
    sal_uLong GetPCDIdx() const;
    void SetPCDIdx( sal_uLong nIdx );
    virtual sal_uLong GetIdx2() const SAL_OVERRIDE;
    virtual void  SetIdx2( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_CP Where() SAL_OVERRIDE;
    virtual void GetSprms( WW8PLCFxDesc* p ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    virtual void Save( WW8PLCFxSave1& rSave ) const SAL_OVERRIDE;
    virtual void Restore( const WW8PLCFxSave1& rSave ) SAL_OVERRIDE;
};

/// Iterator for Piece Table Exceptions of Sepx
class WW8PLCFx_SEPX : public WW8PLCFx
{
private:
    wwSprmParser maSprmParser;
    SvStream* pStrm;
    WW8PLCF* pPLCF;
    sal_uInt8* pSprms;
    sal_uInt16 nArrMax;
    sal_uInt16 nSprmSiz;

    WW8PLCFx_SEPX(const WW8PLCFx_SEPX&) SAL_DELETED_FUNCTION;
    WW8PLCFx_SEPX& operator=(const WW8PLCFx_SEPX&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_SEPX( SvStream* pSt, SvStream* pTablexySt, const WW8Fib& rFib,
        WW8_CP nStartCp );
    virtual ~WW8PLCFx_SEPX();
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual void GetSprms( WW8PLCFxDesc* p ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    const sal_uInt8* HasSprm( sal_uInt16 nId ) const;
    const sal_uInt8* HasSprm( sal_uInt16 nId, sal_uInt8 n2nd ) const;
    const sal_uInt8* HasSprm( sal_uInt16 nId, const sal_uInt8* pOtherSprms,
        long nOtherSprmSiz ) const;
    bool Find4Sprms(sal_uInt16 nId1, sal_uInt16 nId2, sal_uInt16 nId3, sal_uInt16 nId4,
                    sal_uInt8*& p1,   sal_uInt8*& p2,   sal_uInt8*& p3,   sal_uInt8*& p4 ) const;
};

/// iterator for footnotes/endnotes and comments
class WW8PLCFx_SubDoc : public WW8PLCFx
{
private:
    WW8PLCF* pRef;
    WW8PLCF* pText;

    WW8PLCFx_SubDoc(const WW8PLCFx_SubDoc&) SAL_DELETED_FUNCTION;
    WW8PLCFx_SubDoc& operator=(const WW8PLCFx_SubDoc&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_SubDoc(SvStream* pSt, ww::WordVersion eVersion, WW8_CP nStartCp,
    long nFcRef, long nLenRef, long nFcText, long nLenText, long nStruc = 0);
    virtual ~WW8PLCFx_SubDoc();
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;

    // returns reference descriptors
    const void* GetData( long nIdx = -1 ) const
    {
        return pRef ? pRef->GetData( -1L == nIdx ? pRef->GetIdx() : nIdx ) : 0;
    }

    virtual void GetSprms(WW8PLCFxDesc* p) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    long Count() const { return ( pRef ) ? pRef->GetIMax() : 0; }
};

/// Iterator for fields
class WW8PLCFx_FLD : public WW8PLCFx
{
private:
    WW8PLCFspecial* pPLCF;
    const WW8Fib& rFib;
    WW8PLCFx_FLD(const WW8PLCFx_FLD&) SAL_DELETED_FUNCTION;
    WW8PLCFx_FLD& operator=(const WW8PLCFx_FLD &) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_FLD(SvStream* pSt, const WW8Fib& rMyFib, short nType);
    virtual ~WW8PLCFx_FLD();
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual void GetSprms(WW8PLCFxDesc* p) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    bool StartPosIsFieldStart();
    bool EndPosIsFieldEnd(WW8_CP&);
    bool GetPara(long nIdx, WW8FieldDesc& rF);
};

enum eBookStatus { BOOK_NORMAL = 0, BOOK_IGNORE = 0x1, BOOK_FIELD = 0x2 };

/// Iterator for Booknotes
class WW8PLCFx_Book : public WW8PLCFx
{
private:
    WW8PLCFspecial* pBook[2];           // Start and End Position
    std::vector<OUString> aBookNames;   // Name
    std::vector<eBookStatus> aStatus;
    long nIMax;                         // Number of Booknotes
    sal_uInt16 nIsEnd;
    sal_Int32 nBookmarkId; // counter incremented by GetUniqueBookmarkName.

    WW8PLCFx_Book(const WW8PLCFx_Book&) SAL_DELETED_FUNCTION;
    WW8PLCFx_Book& operator=(const WW8PLCFx_Book&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_Book(SvStream* pTableSt,const WW8Fib& rFib);
    virtual ~WW8PLCFx_Book();
    long GetIMax() const { return nIMax; }
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nI ) SAL_OVERRIDE;
    virtual sal_uLong GetIdx2() const SAL_OVERRIDE;
    virtual void SetIdx2( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual long GetNoSprms( WW8_CP& rStart, WW8_CP& rEnd, sal_Int32& rLen ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;
    const OUString* GetName() const;
    WW8_CP GetStartPos() const
        { return ( nIsEnd ) ? WW8_CP_MAX : pBook[0]->Where(); }
    long GetLen() const;
    bool GetIsEnd() const { return nIsEnd != 0; }
    long GetHandle() const;
    void SetStatus( sal_uInt16 nIndex, eBookStatus eStat );
    bool MapName(OUString& rName);
    OUString GetBookmark(long nStart,long nEnd, sal_uInt16 &nIndex);
    eBookStatus GetStatus() const;
    OUString GetUniqueBookmarkName(const OUString &rSuggestedName);
};

/// Handles the import of PlcfAtnBkf and PlcfAtnBkl: start / end position of annotation marks.
class WW8PLCFx_AtnBook : public WW8PLCFx
{
private:
    /// Start and end positions.
    WW8PLCFspecial* m_pBook[2];
    /// Number of annotation marks
    sal_Int32 nIMax;
    bool m_bIsEnd;

    WW8PLCFx_AtnBook(const WW8PLCFx_AtnBook&) SAL_DELETED_FUNCTION;
    WW8PLCFx_AtnBook& operator=(const WW8PLCFx_AtnBook&) SAL_DELETED_FUNCTION;

public:
    WW8PLCFx_AtnBook(SvStream* pTableSt,const WW8Fib& rFib);
    virtual ~WW8PLCFx_AtnBook();
    virtual sal_uInt32 GetIdx() const SAL_OVERRIDE;
    virtual void SetIdx( sal_uLong nI ) SAL_OVERRIDE;
    virtual sal_uLong GetIdx2() const SAL_OVERRIDE;
    virtual void SetIdx2( sal_uLong nIdx ) SAL_OVERRIDE;
    virtual bool SeekPos(WW8_CP nCpPos) SAL_OVERRIDE;
    virtual WW8_FC Where() SAL_OVERRIDE;
    virtual long GetNoSprms( WW8_CP& rStart, WW8_CP& rEnd, sal_Int32& rLen ) SAL_OVERRIDE;
    virtual void advance() SAL_OVERRIDE;

    /// Handle is the unique ID of an annotation mark.
    long getHandle() const;
    bool getIsEnd() const;
};

/*
    this is what we use outside:
*/
struct WW8PLCFManResult
{
    WW8_CP nCpPos;      // attribute starting position
    long nMemLen;       // length for previous
    long nCp2OrIdx;     // footnote-textpos or index in PLCF
    WW8_CP nAktCp;      // only used by caller
    const sal_uInt8* pMemPos;// Mem-Pos for Sprms
    sal_uInt16 nSprmId;     // Sprm-Id ( 0 = invalid Id -> skip! )
                        // (2..255) or pseudo-Sprm-Id (256..260)
                        // from Winword-Ver8 Sprm-Id (800..) resp.
    sal_uInt8 nFlags;        // start of paragraph or section
};

enum ManMaskTypes
{
    MAN_MASK_NEW_PAP = 1,       // new line
    MAN_MASK_NEW_SEP = 2        // new section
};

enum ManTypes // enums for PLCFMan-ctor
{
    MAN_MAINTEXT = 0, MAN_FTN = 1, MAN_EDN = 2, MAN_HDFT = 3, MAN_AND = 4,
    MAN_TXBX = 5, MAN_TXBX_HDFT = 6
};

/*
    this is what the manager uses inside:
*/
struct WW8PLCFxDesc
{
    WW8PLCFx* pPLCFx;
    ::std::stack<sal_uInt16>* pIdStk;  // memory for Attr-Id for Attr-end(s)
    const sal_uInt8* pMemPos;// where are the Sprm(s)
    long nOrigSprmsLen;

    WW8_CP nStartPos;
    WW8_CP nEndPos;

    WW8_CP nOrigStartPos;
    WW8_CP nOrigEndPos;   // The ending character position of a paragraph is
                          // always one before the end reported in the FKP,
                          // also a character run that ends on the same location
                          // as the paragraph mark is adjusted to end just before
                          // the paragraph mark so as to handle their close
                          // first. The value being used to determing where the
                          // properties end is in nEndPos, but the original
                          // unadjusted end character position is important as
                          // it can be used as the beginning cp of the next set
                          // of properties

    WW8_CP nCp2OrIdx;     // where are the NoSprm(s)
    sal_Int32 nSprmsLen;  // how many bytes for further Sprms / length of footnote
    long nCpOfs;          // for Offset Header .. Footnote
    bool bFirstSprm;      // for recognizing the first Sprm of a group
    bool bRealLineEnd;    // false for Pap-Piece-end
    void Save( WW8PLCFxSave1& rSave ) const;
    void Restore( const WW8PLCFxSave1& rSave );
    //With nStartPos set to WW8_CP_MAX then in the case of a pap or chp
    //GetSprms will not search for the sprms, but instead take the
    //existing ones.
    WW8PLCFxDesc()
        : pPLCFx(0)
        , pIdStk(0)
        , pMemPos(0)
        , nOrigSprmsLen(0)
        , nStartPos(WW8_CP_MAX)
        , nEndPos(WW8_CP_MAX)
        , nOrigStartPos(WW8_CP_MAX)
        , nOrigEndPos(WW8_CP_MAX)
        , nCp2OrIdx(WW8_CP_MAX)
        , nSprmsLen(0)
        , nCpOfs(0)
        , bFirstSprm(false)
        , bRealLineEnd(false)
    {
    }
    void ReduceByOffset();
};

#ifndef DUMP

struct WW8PLCFxSaveAll;
class WW8PLCFMan
{
public:
    enum WW8PLCFManLimits {MAN_ANZ_PLCF = 11};

private:
    wwSprmParser maSprmParser;
    long nCpO;                      // Origin Cp -- the basis for nNewCp

    WW8_CP nLineEnd;                // points *after* the <CR>
    long nLastWhereIdxCp;           // last result of WhereIdx()
    sal_uInt16 nPLCF;                   // this many PLCFs are managed
    ManTypes nManType;
    bool mbDoingDrawTextBox;        //Normally we adjust the end of attributes
                                    //so that the end of a paragraph occurs
                                    //before the para end mark, but for
                                    //drawboxes we want the true offsets

    WW8PLCFxDesc aD[MAN_ANZ_PLCF];
    WW8PLCFxDesc *pChp, *pPap, *pSep, *pField, *pFootnote, *pEdn, *pBkm, *pPcd,
        *pPcdA, *pAnd, *pAtnBkm;
    WW8PLCFspecial *pFdoa, *pTxbx, *pTxbxBkd,*pMagicTables, *pSubdocs;
    sal_uInt8* pExtendedAtrds;

    const WW8Fib* pWwFib;

    sal_uInt16 WhereIdx(bool* pbStart=0, long* pPos=0) const;
    void AdjustEnds(WW8PLCFxDesc& rDesc);
    void GetNewSprms(WW8PLCFxDesc& rDesc);
    static void GetNewNoSprms(WW8PLCFxDesc& rDesc);
    void GetSprmStart(short nIdx, WW8PLCFManResult* pRes) const;
    void GetSprmEnd(short nIdx, WW8PLCFManResult* pRes) const;
    void GetNoSprmStart(short nIdx, WW8PLCFManResult* pRes) const;
    void GetNoSprmEnd(short nIdx, WW8PLCFManResult* pRes) const;
    void AdvSprm(short nIdx, bool bStart);
    void AdvNoSprm(short nIdx, bool bStart);
    sal_uInt16 GetId(const WW8PLCFxDesc* p ) const;

public:
    WW8PLCFMan(WW8ScannerBase* pBase, ManTypes nType, long nStartCp,
        bool bDoingDrawTextBox = false);
    ~WW8PLCFMan();

    /*
        Where asks on which following position any Attr changes...
    */
    WW8_CP Where() const;

    bool Get(WW8PLCFManResult* pResult) const;
    void advance();
    sal_uInt16 GetColl() const; // index of actual Style
    WW8PLCFx_FLD* GetField() const;
    WW8PLCFx_SubDoc* GetEdn() const { return static_cast<WW8PLCFx_SubDoc*>(pEdn->pPLCFx); }
    WW8PLCFx_SubDoc* GetFootnote() const { return static_cast<WW8PLCFx_SubDoc*>(pFootnote->pPLCFx); }
    WW8PLCFx_SubDoc* GetAtn() const { return static_cast<WW8PLCFx_SubDoc*>(pAnd->pPLCFx); }
    WW8PLCFx_Book* GetBook() const { return static_cast<WW8PLCFx_Book*>(pBkm->pPLCFx); }
    WW8PLCFx_AtnBook* GetAtnBook() const { return static_cast<WW8PLCFx_AtnBook*>(pAtnBkm->pPLCFx); }
    long GetCpOfs() const { return pChp->nCpOfs; }  // for Header/Footer...

    /* asks, if *current paragraph* has an Sprm of this type */
    const sal_uInt8* HasParaSprm( sal_uInt16 nId ) const;

    /* asks, if *current textrun* has an Sprm of this type */
    const sal_uInt8* HasCharSprm( sal_uInt16 nId ) const;
    bool HasCharSprm(sal_uInt16 nId, std::vector<const sal_uInt8 *> &rResult) const;

    WW8PLCFx_Cp_FKP* GetChpPLCF() const
        { return static_cast<WW8PLCFx_Cp_FKP*>(pChp->pPLCFx); }
    WW8PLCFx_Cp_FKP* GetPapPLCF() const
        { return static_cast<WW8PLCFx_Cp_FKP*>(pPap->pPLCFx); }
    WW8PLCFx_SEPX* GetSepPLCF() const
        { return static_cast<WW8PLCFx_SEPX*>(pSep->pPLCFx); }
    WW8PLCFxDesc* GetPap() const { return pPap; }
    bool TransferOpenSprms(std::stack<sal_uInt16> &rStack);
    void SeekPos( long nNewCp );
    void SaveAllPLCFx( WW8PLCFxSaveAll& rSave ) const;
    void RestoreAllPLCFx( const WW8PLCFxSaveAll& rSave );
    WW8PLCFspecial* GetFdoa() const { return pFdoa; }
    WW8PLCFspecial* GetTxbx() const { return pTxbx; }
    WW8PLCFspecial* GetTxbxBkd() const { return pTxbxBkd; }
    WW8PLCFspecial* GetMagicTables() const { return pMagicTables; }
    WW8PLCFspecial* GetWkbPLCF() const { return pSubdocs; }
    sal_uInt8* GetExtendedAtrds() const { return pExtendedAtrds; }
    ManTypes GetManType() const { return nManType; }
    bool GetDoingDrawTextBox() const { return mbDoingDrawTextBox; }
};

struct WW8PLCFxSaveAll
{
    WW8PLCFxSave1 aS[WW8PLCFMan::MAN_ANZ_PLCF];
    WW8PLCFxSaveAll()
    {
        memset(aS, 0, sizeof(aS));
    }
};

#endif // !DUMP

class WW8ScannerBase
{
friend WW8PLCFx_PCDAttrs::WW8PLCFx_PCDAttrs(ww::WordVersion eVersion,
    WW8PLCFx_PCD* pPLCFx_PCD, const WW8ScannerBase* pBase );
friend WW8PLCFx_Cp_FKP::WW8PLCFx_Cp_FKP( SvStream*, SvStream*, SvStream*,
    const WW8ScannerBase&, ePLCFT );

#ifndef DUMP
friend WW8PLCFMan::WW8PLCFMan(WW8ScannerBase*, ManTypes, long, bool);
friend class SwWW8FltControlStack;
#endif

private:
    WW8Fib* pWw8Fib;
    WW8PLCFx_Cp_FKP*  pChpPLCF;         // Character-Attrs
    WW8PLCFx_Cp_FKP*  pPapPLCF;         // Paragraph-Attrs
    WW8PLCFx_SEPX*    pSepPLCF;         // Section-Attrs
    WW8PLCFx_SubDoc*  pFootnotePLCF;         // Footnotes
    WW8PLCFx_SubDoc*  pEdnPLCF;         // EndNotes
    WW8PLCFx_SubDoc*  pAndPLCF;         // Comments
    WW8PLCFx_FLD*     pFieldPLCF;         // Fields in Main Text
    WW8PLCFx_FLD*     pFieldHdFtPLCF;     // Fields in Header / Footer
    WW8PLCFx_FLD*     pFieldTxbxPLCF;     // Fields in Textboxes in Main Text
    WW8PLCFx_FLD*     pFieldTxbxHdFtPLCF; // Fields in Textboxes in Header / Footer
    WW8PLCFx_FLD*     pFieldFootnotePLCF;      // Fields in Footnotes
    WW8PLCFx_FLD*     pFieldEdnPLCF;      // Fields in Endnotes
    WW8PLCFx_FLD*     pFieldAndPLCF;      // Fields in Comments
    WW8PLCFspecial*   pMainFdoa;        // Graphic Primitives in Main Text
    WW8PLCFspecial*   pHdFtFdoa;        // Graphic Primitives in Header / Footer
    WW8PLCFspecial*   pMainTxbx;        // Textboxes in Main Text
    WW8PLCFspecial*   pMainTxbxBkd;     // Break-Descriptors for them
    WW8PLCFspecial*   pHdFtTxbx;        // TextBoxes in Header / Footer
    WW8PLCFspecial*   pHdFtTxbxBkd;     // Break-Descriptors for previous
    WW8PLCFspecial*   pMagicTables;     // Break-Descriptors for them
    WW8PLCFspecial*   pSubdocs;         // subdoc references in master document
    sal_uInt8*        pExtendedAtrds;   // Extended ATRDs
    WW8PLCFx_Book*    pBook;            // Bookmarks
    WW8PLCFx_AtnBook* pAtnBook;         // Annotationmarks

    WW8PLCFpcd*         pPiecePLCF; // for FastSave ( Basis-PLCF without iterator )
    WW8PLCFpcd_Iter*    pPieceIter; // for FastSave ( iterator for previous )
    WW8PLCFx_PCD*       pPLCFx_PCD;     // ditto
    WW8PLCFx_PCDAttrs*  pPLCFx_PCDAttrs;
    std::vector<sal_uInt8*> aPieceGrpprls;  // attributes of Piece-Table

    WW8PLCFpcd* OpenPieceTable( SvStream* pStr, const WW8Fib* pWwF );
    void DeletePieceTable();

public:
    WW8ScannerBase( SvStream* pSt, SvStream* pTableSt, SvStream* pDataSt,
        WW8Fib* pWwF );
    ~WW8ScannerBase();
    bool AreThereFootnotes() const { return pFootnotePLCF->Count() > 0; };
    bool AreThereEndnotes()  const { return pEdnPLCF->Count() > 0; };

    //If you use WW8Fc2Cp you are almost certainly doing the wrong thing
    //when it comes to fastsaved files, avoid like the plague. For export
    //given that we never write fastsaved files you can use it, otherwise
    //I will beat you with a stick
    WW8_CP WW8Fc2Cp(WW8_FC nFcPos) const ;
    WW8_FC WW8Cp2Fc(WW8_CP nCpPos, bool* pIsUnicode = 0,
        WW8_CP* pNextPieceCp = 0, bool* pTestFlag = 0) const;

    sal_Int32 WW8ReadString(SvStream& rStrm, OUString& rStr, WW8_CP nAktStartCp,
        long nTotalLen, rtl_TextEncoding eEnc ) const;

};

/** FIB - the File Information Block

    The FIB contains a "magic word" and pointers to the various other parts of
    the file, as well as information about the length of the file.
    The FIB starts at the beginning of the file.
*/
class WW8Fib
{
private:
    sal_Unicode nNumDecimalSep;

public:
    /**
        Program-Version asked for by us:
        in Ctor we check if it matches the value of nFib

        6 == "WinWord 6 or WinWord 95",
        7 == "only WinWord 95"
        8 == "WinWord 97 or newer"
    */
    sal_uInt8 nVersion;
    /*
        error status
    */
    sal_uLong nFibError;
    /*
        data read from FIB by Ctor
        (corresponds only approximately to the real structure
        of the Winword-FIB)
    */
    sal_uInt16 wIdent;      // 0x0 int magic number
    /*
        File Information Block (FIB) values:
        WinWord 1.0 = 33
        WinWord 2.0 = 45
        WinWord 6.0c for 16bit = 101
        Word 6/32 bit = 104
        Word 95 = 104
        Word 97 = 193
        Word 2000 = 217
        Word 2002 = 257
        Word 2003 = 268
        Word 2007 = 274
    */
    sal_uInt16 nFib;        // 0x2 FIB version written
    sal_uInt16 nProduct;    // 0x4 product version written by
    sal_Int16 lid;          // 0x6 language stamp---localized version;
    WW8_PN pnNext;          // 0x8

    bool fDot :1;     // 0xa 0001
    bool fGlsy :1;
    bool fComplex :1; // 0004 when 1, file is in complex, fast-saved format.
    bool fHasPic :1;  // 0008 file contains 1 or more pictures
    sal_uInt16 cQuickSaves :4; // 00F0 count of times file was quicksaved
    bool fEncrypted :1; //0100 1 if file is encrypted, 0 if not
    bool fWhichTableStm :1; //0200 When 0, this fib refers to the table stream
    bool fReadOnlyRecommended :1;
    bool fWriteReservation :1;
                                                    // named "0Table", when 1, this fib refers to the
                                                    // table stream named "1Table". Normally, a file
                                                    // will have only one table stream, but under unusual
                                                    // circumstances a file may have table streams with
                                                    // both names. In that case, this flag must be used
                                                    // to decide which table stream is valid.

    bool fExtChar :1; // 1000 =1, when using extended character set in file
    bool fFarEast :1; // 4000 =1, probably, when far-East language vaiants of Word is used to create a file #i90932#

    bool fObfuscated :1; // 8000=1. specifies whether the document is obfuscated using XOR obfuscation. otherwise this bit MUST be ignored.

    sal_uInt16 nFibBack;    // 0xc
    sal_uInt16 nHash;       // 0xe  file encrypted hash
    sal_uInt16 nKey;        // 0x10  file encrypted key
    sal_uInt8 envr;         // 0x12 environment in which file was created
                                    //      0 created by Win Word / 1 created by Mac Word
    bool fMac              :1;          // 0x13 when 1, this file was last saved in the Mac environment
    bool fEmptySpecial     :1;
    bool fLoadOverridePage :1;
    bool fFuturesavedUndo  :1;
    bool fWord97Saved      :1;
    bool fWord2000Saved    :1;
    sal_uInt8 :2;

    sal_uInt16 chse;        // 0x14 default extended character set id for text in document stream. (overidden by chp.chse)
                        //      0 = ANSI  / 256 Macintosh character set.
    sal_uInt16 chseTables;  // 0x16 default extended character set id for text in
                        //      internal data structures: 0 = ANSI, 256 = Macintosh
    WW8_FC fcMin;           // 0x18 file offset of first character of text
    WW8_FC fcMac;           // 0x1c file offset of last character of text + 1

    // start of WW8 section
    sal_uInt16 csw;             // Count of fields in the array of "shorts"

    // marker: "rgsw" Beginning of the array of shorts
    sal_uInt16 wMagicCreated;   // unique number Identifying the File's creator
                                // 0x6A62 is the creator ID for Word and is reserved.
                                // Other creators should choose a different value.
    sal_uInt16 wMagicRevised;   // identifies the File's last modifier
    sal_uInt16 wMagicCreatedPrivate; // private data
    sal_uInt16 wMagicRevisedPrivate; // private data

    sal_Int16  lidFE; // Language id if document was written by Far East version
                      // of Word (i.e. FIB.fFarEast is on)
    sal_uInt16 clw; // Number of fields in the array of longs

    // end of WW8 section

    // Marker: "rglw" Beginning of the array of longs
    WW8_FC cbMac;           // 0x20 file offset of last byte written to file + 1.

    // WW8_FC u4[4];        // 0x24
    WW8_CP ccpText;         // 0x34 length of main document text stream
    WW8_CP ccpFootnote;          // 0x38 length of footnote subdocument text stream
    WW8_CP ccpHdr;          // 0x3c length of header subdocument text stream
    WW8_CP ccpMcr;          // 0x40 length of macro subdocument text stream
    WW8_CP ccpAtn;          // 0x44 length of annotation subdocument text stream
    WW8_CP ccpEdn;          // 0x48 length of endnote subdocument text stream
    WW8_CP ccpTxbx;         // 0x4c length of textbox subdocument text stream
    WW8_CP ccpHdrTxbx;      // 0x50 length of header textbox subdocument text stream

    // start of WW8 section
    sal_Int32  pnFbpChpFirst; // when there was insufficient memory for Word to expand
                              // the PLCFbte at save time, the PLCFbte is written
                              // to the file in a linked list of 512-byte pieces
                              // starting with this pn.
    sal_Int32  pnFbpPapFirst; // when there was insufficient memory for Word to expand
                              // the PLCFbte at save time, the PLCFbte is written to
                              // the file in a linked list of 512-byte pieces
                              // starting with this pn

    sal_Int32  pnFbpLvcFirst; // when there was insufficient memory for Word to expand
                              // the PLCFbte at save time, the PLCFbte is written to
                              // the file in a linked list of 512-byte pieces
                              // starting with this pn
    sal_Int32  pnLvcFirst; // the page number of the lowest numbered page in the
                           // document that records LVC FKP information
    sal_Int32  cpnBteLvc;  // count of LVC FKPs recorded in file. In non-complex
                           // files if the number of entries in the PLCFbtePapx is
                           // less than this, the PLCFbtePapx is incomplete.
    sal_Int32  fcIslandFirst;   // ?
    sal_Int32  fcIslandLim;     // ?
    sal_uInt16 cfclcb; // Number of fields in the array of FC/LCB pairs.

    // end of WW8 section

    // Marker: "rgfclcb" Beginning of array of FC/LCB pairs.
    WW8_FC fcStshfOrig;     // file offset of original allocation for STSH in table
                                                // stream. During fast save Word will attempt to reuse
                                                // this allocation if STSH is small enough to fit.
    sal_Int32 lcbStshfOrig; // 0x5c count of bytes of original STSH allocation
    WW8_FC fcStshf;         // 0x60 file offset of STSH in file.
    sal_Int32 lcbStshf;     // 0x64 count of bytes of current STSH allocation
    WW8_FC fcPlcffndRef;    // 0x68 file offset of footnote reference PLCF.
    sal_Int32 lcbPlcffndRef;    // 0x6c count of bytes of footnote reference PLCF
                        //      == 0 if no footnotes defined in document.

    WW8_FC fcPlcffndText;    // 0x70 file offset of footnote text PLCF.
    sal_Int32 lcbPlcffndText;    // 0x74 count of bytes of footnote text PLCF.
                        //      == 0 if no footnotes defined in document

    WW8_FC fcPlcfandRef;    // 0x78 file offset of annotation reference PLCF.
    sal_Int32 lcbPlcfandRef;    // 0x7c count of bytes of annotation reference PLCF.

    WW8_FC fcPlcfandText;    // 0x80 file offset of annotation text PLCF.
    sal_Int32 lcbPlcfandText;    // 0x84 count of bytes of the annotation text PLCF

    WW8_FC fcPlcfsed;       // 8x88 file offset of section descriptor PLCF.
    sal_Int32 lcbPlcfsed;   // 0x8c count of bytes of section descriptor PLCF.

    WW8_FC fcPlcfpad;       // 0x90 file offset of paragraph descriptor PLCF
    sal_Int32 lcbPlcfpad;   // 0x94 count of bytes of paragraph descriptor PLCF.
                        // ==0 if file was never viewed in Outline view.
                        // Should not be written by third party creators

    WW8_FC fcPlcfphe;       // 0x98 file offset of PLCF of paragraph heights.
    sal_Int32 lcbPlcfphe;   // 0x9c count of bytes of paragraph height PLCF.
                        // ==0 when file is non-complex.

    WW8_FC fcSttbfglsy;     // 0xa0 file offset of glossary string table.
    sal_Int32 lcbSttbfglsy; // 0xa4 count of bytes of glossary string table.
                        //      == 0 for non-glossary documents.
                        //      !=0 for glossary documents.

    WW8_FC fcPlcfglsy;      // 0xa8 file offset of glossary PLCF.
    sal_Int32 lcbPlcfglsy;  // 0xac count of bytes of glossary PLCF.
                        //      == 0 for non-glossary documents.
                        //      !=0 for glossary documents.

    WW8_FC fcPlcfhdd;       // 0xb0 byte offset of header PLCF.
    sal_Int32 lcbPlcfhdd;   // 0xb4 count of bytes of header PLCF.
                        //      == 0 if document contains no headers

    WW8_FC fcPlcfbteChpx;   // 0xb8 file offset of character property bin table.PLCF.
    sal_Int32 lcbPlcfbteChpx;// 0xbc count of bytes of character property bin table PLCF.

    WW8_FC fcPlcfbtePapx;   // 0xc0 file offset of paragraph property bin table.PLCF.
    sal_Int32 lcbPlcfbtePapx;// 0xc4 count of bytes of paragraph  property bin table PLCF.

    WW8_FC fcPlcfsea;       // 0xc8 file offset of PLCF reserved for private use. The SEA is 6 bytes long.
    sal_Int32 lcbPlcfsea;   // 0xcc count of bytes of private use PLCF.

    WW8_FC fcSttbfffn;      // 0xd0 file offset of font information STTBF. See the FFN file structure definition.
    sal_Int32 lcbSttbfffn;  // 0xd4 count of bytes in sttbfffn.

    WW8_FC fcPlcffldMom;    // 0xd8 offset in doc stream to the PLCF of field positions in the main document.
    sal_Int32 lcbPlcffldMom;    // 0xdc

    WW8_FC fcPlcffldHdr;    // 0xe0 offset in doc stream to the PLCF of field positions in the header subdocument.
    sal_Int32 lcbPlcffldHdr;    // 0xe4

    WW8_FC fcPlcffldFootnote;    // 0xe8 offset in doc stream to the PLCF of field positions in the footnote subdocument.
    sal_Int32 lcbPlcffldFootnote;    // 0xec

    WW8_FC fcPlcffldAtn;    // 0xf0 offset in doc stream to the PLCF of field positions in the annotation subdocument.
    sal_Int32 lcbPlcffldAtn;    // 0xf4

    WW8_FC fcPlcffldMcr;    // 0xf8 offset in doc stream to the PLCF of field positions in the macro subdocument.
    sal_Int32 lcbPlcffldMcr;    // 9xfc

    WW8_FC fcSttbfbkmk; // 0x100 offset in document stream of the STTBF that records bookmark names in the main document
    sal_Int32 lcbSttbfbkmk; // 0x104

    WW8_FC fcPlcfbkf;   // 0x108 offset in document stream of the PLCF that records the beginning CP offsets of bookmarks in the main document. See BKF
    sal_Int32 lcbPlcfbkf;   // 0x10c

    WW8_FC fcPlcfbkl;   // 0x110 offset in document stream of the PLCF that records the ending CP offsets of bookmarks recorded in the main document. See the BKL structure definition.
    sal_Int32 lcbPlcfbkl;   // 0x114 sal_Int32

    WW8_FC fcCmds;      // 0x118 FC
    sal_Int32 lcbCmds;      // 0x11c

    WW8_FC fcPlcfmcr;       // 0x120 FC
    sal_Int32 lcbPlcfmcr;       // 0x124

    WW8_FC fcSttbfmcr;  // 0x128 FC
    sal_Int32 lcbSttbfmcr;  // 0x12c

    WW8_FC fcPrDrvr;        // 0x130 file offset of the printer driver information (names of drivers, port etc...)
    sal_Int32 lcbPrDrvr;        // 0x134 count of bytes of the printer driver information (names of drivers, port etc...)

    WW8_FC fcPrEnvPort; // 0x138 file offset of the print environment in portrait mode.
    sal_Int32 lcbPrEnvPort; // 0x13c count of bytes of the print environment in portrait mode.

    WW8_FC fcPrEnvLand; // 0x140 file offset of the print environment in landscape mode.
    sal_Int32 lcbPrEnvLand; // 0x144 count of bytes of the print environment in landscape mode.

    WW8_FC fcWss;       // 0x148 file offset of Window Save State data structure. See WSS.
    sal_Int32 lcbWss;       // 0x14c count of bytes of WSS. ==0 if unable to store the window state.

    WW8_FC fcDop;       // 0x150 file offset of document property data structure.
    sal_uInt32 lcbDop;       // 0x154 count of bytes of document properties.
        // cbDOP is 84 when nFib < 103

    WW8_FC fcSttbfAssoc;    // 0x158 offset to STTBF of associated strings. See STTBFASSOC.
    sal_Int32 lcbSttbfAssoc; // 0x15C

    WW8_FC fcClx;           // 0x160 file  offset of beginning of information for complex files.
    sal_Int32 lcbClx;       // 0x164 count of bytes of complex file information. 0 if file is non-complex.

    WW8_FC fcPlcfpgdFootnote;    // 0x168 file offset of page descriptor PLCF for footnote subdocument.
    sal_Int32 lcbPlcfpgdFootnote;    // 0x16C count of bytes of page descriptor PLCF for footnote subdocument.
                        //  ==0 if document has not been paginated. The length of the PGD is 8 bytes.

    WW8_FC fcAutosaveSource;    // 0x170 file offset of the name of the original file.
    sal_Int32 lcbAutosaveSource;    // 0x174 count of bytes of the name of the original file.

    WW8_FC fcGrpStAtnOwners;    // 0x178 group of strings recording the names of the owners of annotations
    sal_Int32 lcbGrpStAtnOwners;    // 0x17C count of bytes of the group of strings

    WW8_FC fcSttbfAtnbkmk;  // 0x180 file offset of the sttbf that records names of bookmarks in the annotation subdocument
    sal_Int32 lcbSttbfAtnbkmk;  // 0x184 length in bytes of the sttbf that records names of bookmarks in the annotation subdocument

    // end of WW67 section

    WW8_FC fcPlcfdoaMom;    // 0x192 file offset of the  FDOA (drawn object) PLCF for main document.
                        //  ==0 if document has no drawn objects. The length of the FDOA is 6 bytes.
                        // unused starting from Ver8
    sal_Int32 lcbPlcfdoaMom;    // 0x196 length in bytes of the FDOA PLCF of the main document
                                                // unused starting from Ver8
    WW8_FC fcPlcfdoaHdr;    // 0x19A file offset of the  FDOA (drawn object) PLCF for the header document.
                        //  ==0 if document has no drawn objects. The length of the FDOA is 6 bytes.
                        // unused starting from Ver8
    sal_Int32 lcbPlcfdoaHdr;    // 0x19E length in bytes of the FDOA PLCF of the header document
                                                // unused starting from Ver8

    WW8_FC fcPlcfspaMom;        // offset in table stream of the FSPA PLCF for main document.
                                                // == 0 if document has no office art objects
                                                        // was empty reserve in Ver67
    sal_Int32 lcbPlcfspaMom;        // length in bytes of the FSPA PLCF of the main document
                                                        // was empty reserve in Ver67
    WW8_FC fcPlcfspaHdr;        // offset in table stream of the FSPA PLCF for header document.
                                                // == 0 if document has no office art objects
                                                        // was empty reserve in Ver67
    sal_Int32 lcbPlcfspaHdr;        // length in bytes of the FSPA PLCF of the header document
                                                        // was empty reserve in Ver67

    WW8_FC fcPlcfAtnbkf;    // 0x1B2 file offset of BKF (bookmark first) PLCF of the annotation subdocument
    sal_Int32 lcbPlcfAtnbkf;    // 0x1B6 length in bytes of BKF (bookmark first) PLCF of the annotation subdocument

    WW8_FC fcPlcfAtnbkl;    // 0x1BA file offset of BKL (bookmark last) PLCF of the annotation subdocument
    sal_Int32 lcbPlcfAtnbkl;    // 0x1BE length in bytes of BKL (bookmark first) PLCF of the annotation subdocument

    WW8_FC fcPms;       // 0x1C2 file offset of PMS (Print Merge State) information block
    sal_Int32 lcbPMS;       // 0x1C6 length in bytes of PMS

    WW8_FC fcFormFieldSttbf;  // 0x1CA file offset of form field Sttbf which contains strings used in form field dropdown controls
    sal_Int32 lcbFormFieldSttbf;  // 0x1CE length in bytes of form field Sttbf

    WW8_FC fcPlcfendRef;    // 0x1D2 file offset of PLCFendRef which points to endnote references in the main document stream
    sal_Int32 lcbPlcfendRef;    // 0x1D6

    WW8_FC fcPlcfendText;    // 0x1DA file offset of PLCFendRef which points to endnote text  in the endnote document
                        //       stream which corresponds with the PLCFendRef
    sal_Int32 lcbPlcfendText;    // 0x1DE

    WW8_FC fcPlcffldEdn;    // 0x1E2 offset to PLCF of field positions in the endnote subdoc
    sal_Int32 lcbPlcffldEdn;    // 0x1E6

    WW8_FC  fcPlcfpgdEdn;   // 0x1EA offset to PLCF of page boundaries in the endnote subdoc.
    sal_Int32 lcbPlcfpgdEdn;        // 0x1EE

    WW8_FC fcDggInfo;           // offset in table stream of the office art object table data.
                                                // The format of office art object table data is found in a separate document.
                                                        // was empty reserve in Ver67
    sal_Int32 lcbDggInfo;           // length in bytes of the office art object table data
                                                        // was empty reserve in Ver67

    WW8_FC fcSttbfRMark;        // 0x1fa offset to STTBF that records the author abbreviations...
    sal_Int32 lcbSttbfRMark;        // 0x1fe
    WW8_FC fcSttbfCaption;  // 0x202 offset to STTBF that records caption titles...
    sal_Int32 lcbSttbfCaption;  // 0x206
    WW8_FC fcSttbAutoCaption;   // offset in table stream to the STTBF that records the object names and
                                                        // indices into the caption STTBF for objects which get auto captions.
    sal_Int32 lcbSttbAutoCaption;   // 0x20e

    WW8_FC fcPlcfwkb;       // 0x212 offset to PLCF that describes the boundaries of contributing documents...
    sal_Int32 lcbPlcfwkb;       // 0x216

    WW8_FC fcPlcfspl;       // offset in table stream of PLCF (of SPLS structures) that records spell check state
                                                        // was empty reserve in Ver67
    sal_Int32 lcbPlcfspl;                   // was empty reserve in Ver67

    WW8_FC fcPlcftxbxText;   // 0x222 ...PLCF of beginning CP in the text box subdoc
    sal_Int32 lcbPlcftxbxText;   // 0x226
    WW8_FC fcPlcffldTxbx;   // 0x22a ...PLCF of field boundaries recorded in the textbox subdoc.
    sal_Int32 lcbPlcffldTxbx;   // 0x22e
    WW8_FC fcPlcfHdrtxbxText;// 0x232 ...PLCF of beginning CP in the header text box subdoc
    sal_Int32 lcbPlcfHdrtxbxText;// 0x236
    WW8_FC fcPlcffldHdrTxbx;// 0x23a ...PLCF of field boundaries recorded in the header textbox subdoc.
    sal_Int32 lcbPlcffldHdrTxbx;// 0x23e
    WW8_FC fcStwUser;
    sal_uInt32 lcbStwUser;
    WW8_FC fcSttbttmbd;
    sal_uInt32 lcbSttbttmbd;

    WW8_FC fcSttbFnm;       // 0x02da offset in the table stream of masters subdocument names
    sal_Int32 lcbSttbFnm;       // 0x02de length

    /*
        special list handling for WW8
    */
    WW8_FC fcPlcfLst;       // 0x02e2 offset in the table stream of list format information.
    sal_Int32 lcbPlcfLst;       // 0x02e6 length
    WW8_FC fcPlfLfo;        // 0x02ea offset in the table stream of list format override information.
    sal_Int32 lcbPlfLfo;        // 0x02ee length
    /*
        special Break handling for text-box-stories in WW8
    */
    WW8_FC fcPlcftxbxBkd;   // 0x02f2 PLCF for TextBox-Break-descriptors in the Maintext
    sal_Int32 lcbPlcftxbxBkd;   // 0x02f6
    WW8_FC fcPlcfHdrtxbxBkd;// 0x02fa PLCF for TextBox-Break-descriptors in the Header-/Footer- area
    sal_Int32 lcbPlcfHdrtxbxBkd;// 0x02fe

    // 0x302 - 372 == ignore
    /*
        ListNames (skip to here!)
    */
    WW8_FC fcSttbListNames;// 0x0372 PLCF for Listname Table
    sal_Int32 lcbSttbListNames;// 0x0376

    WW8_FC fcPlcfTch;
    sal_Int32 lcbPlcfTch;

    // 0x38A - 41A == ignore
    WW8_FC fcAtrdExtra;
    sal_uInt32 lcbAtrdExtra;

    // 0x422 - 0x4D4 == ignore
    WW8_FC fcHplxsdr;    //bizarrely, word xp seems to require this set to shows dates from AtrdExtra
    sal_uInt32 lcbHplxsdr;

    /*
        general variables that were used for Ver67 and Ver8,
        even though they had different sizes in the corresponding files:
    */
    sal_Int32 pnChpFirst;
    sal_Int32 pnPapFirst;
    sal_Int32 cpnBteChp;
    sal_Int32 cpnBtePap;
    /*
        The actual nFib, moved here because some readers assumed
        they couldn't read any format with nFib > some constant
    */
    sal_uInt16 nFib_actual; // 0x05bc #i56856#
    /*
        now we only need a Ctor
    */
    WW8Fib( SvStream& rStrm, sal_uInt8 nWantedVersion,sal_uInt32 nOffset=0 );

    /* unfortunately incorrect, you still need one for the export */
    WW8Fib( sal_uInt8 nVersion = 6, bool bDot = false );
    bool WriteHeader(SvStream& rStrm);
    bool Write(SvStream& rStrm);
    static rtl_TextEncoding GetFIBCharset(sal_uInt16 chs, sal_uInt16 nLidLocale);
    ww::WordVersion GetFIBVersion() const;
    WW8_CP GetBaseCp(ManTypes nType) const;
    sal_Unicode getNumDecimalSep() const { return nNumDecimalSep;}
};

class WW8Style
{
protected:
    WW8Fib& rFib;
    SvStream& rSt;

    sal_uInt16  cstd;                      // Count of styles in stylesheet
    sal_uInt16  cbSTDBaseInFile;           // Length of STD Base as stored in a file
    sal_uInt16  fStdStylenamesWritten : 1; // Are built-in stylenames stored?
    sal_uInt16  : 15;                      // Spare flags
    sal_uInt16  stiMaxWhenSaved;           // Max sti known when file was written
    sal_uInt16  istdMaxFixedWhenSaved;     // How many fixed-index istds are there?
    sal_uInt16  nVerBuiltInNamesWhenSaved; // Current version of built-in stylenames
    // ftc used by StandardChpStsh for this document
    sal_uInt16  ftcAsci;
    // CJK ftc used by StandardChpStsh for this document
    sal_uInt16  ftcFE;
    // CTL/Other ftc used by StandardChpStsh for this document
    sal_uInt16  ftcOther;
    // CTL ftc used by StandardChpStsh for this document
    sal_uInt16  ftcBi;

    //No copying
    WW8Style(const WW8Style&);
    WW8Style& operator=(const WW8Style&);

public:
    WW8Style( SvStream& rSt, WW8Fib& rFibPara );
    WW8_STD* Read1STDFixed( short& rSkip, short* pcbStd );
    WW8_STD* Read1Style( short& rSkip, OUString* pString, short* pcbStd );
    sal_uInt16 GetCount() const { return cstd; }
};

class WW8Fonts
{
protected:
    WW8_FFN* pFontA;    // Array of Pointers to Font Description
    sal_uInt16 nMax;        // Array-Size

public:
    WW8Fonts( SvStream& rSt, WW8Fib& rFib );
    ~WW8Fonts() { delete[] pFontA; }
    const WW8_FFN* GetFont( sal_uInt16 nNum ) const;
    sal_uInt16 GetMax() const { return nMax; }
};

typedef sal_uInt8 HdFtFlags;
namespace nsHdFtFlags
{
    const HdFtFlags WW8_HEADER_EVEN     = 0x01;
    const HdFtFlags WW8_HEADER_ODD      = 0x02;
    const HdFtFlags WW8_FOOTER_EVEN     = 0x04;
    const HdFtFlags WW8_FOOTER_ODD      = 0x08;
    const HdFtFlags WW8_HEADER_FIRST    = 0x10;
    const HdFtFlags WW8_FOOTER_FIRST    = 0x20;
}

/// Document Properties
struct WW8Dop
{
public:
    /* Error Status */
    sal_uLong nDopError;
    /*
    Corresponds only roughly to the actual structure of the Winword DOP,
    the winword FIB version matters to what exists.
    */
    bool        fFacingPages : 1;   // 1 when facing pages should be printed

    bool        fWidowControl : 1;  //a: orig 97 docs say
                                    //    1 when widow control is in effect. 0 when widow control disabled.
                                    //b: MS-DOC: Word Binary File Format (.doc) Structure Specification 2008 says
                                    //    B - unused1 (1 bit): Undefined and MUST be ignored.

    bool        fPMHMainDoc : 1;    // 1 when doc is a main doc for Print Merge Helper, 0 when not; default=0
    sal_uInt16  grfSuppression : 2; // 0 Default line suppression storage; 0= form letter line suppression; 1= no line suppression; default=0
    sal_uInt16  fpc : 2;            // 1 footnote position code: 0 as endnotes, 1 at bottom of page, 2 immediately beneath text
    sal_uInt16  : 1;                // 0 unused

    sal_uInt16  grpfIhdt : 8;           // 0 specification of document headers and footers. See explanation under Headers and Footers topic.

    sal_uInt16  rncFootnote : 2;         // 0 restart index for footnotes, 0 don't restart note numbering, 1 section, 2 page
    sal_uInt16  nFootnote : 14;          // 1 initial footnote number for document
    bool        fOutlineDirtySave : 1; // when 1, indicates that information in the hPLCFpad should be refreshed since outline has been dirtied
    sal_uInt16  : 7;                //   reserved
    bool        fOnlyMacPics : 1;   //   when 1, Word believes all pictures recorded in the document were created on a Macintosh
    bool        fOnlyWinPics : 1;   //   when 1, Word believes all pictures recorded in the document were created in Windows
    bool        fLabelDoc : 1;      //   when 1, document was created as a print merge labels document
    bool        fHyphCapitals : 1;  //   when 1, Word is allowed to hyphenate words that are capitalized. When 0, capitalized may not be hyphenated
    bool        fAutoHyphen : 1;    //   when 1, Word will hyphenate newly typed text as a background task
    bool        fFormNoFields : 1;
    bool        fLinkStyles : 1;    //   when 1, Word will merge styles from its template
    bool        fRevMarking : 1;    //   when 1, Word will mark revisions as the document is edited
    bool        fBackup : 1;        //   always make backup when document saved when 1.
    bool        fExactCWords : 1;
    bool        fPagHidden : 1;
    bool        fPagResults : 1;
    bool        fLockAtn : 1;       //   when 1, annotations are locked for editing
    bool        fMirrorMargins : 1; //   swap margins on left/right pages when 1.
    bool        fReadOnlyRecommended : 1;// user has recommended that this doc be opened read-only when 1
    bool        fDfltTrueType : 1;  //   when 1, use TrueType fonts by default (flag obeyed only when doc was created by WinWord 2.x)
    bool        fPagSuppressTopSpacing : 1;//when 1, file created with SUPPRESSTOPSPACING=YES in win.ini. (flag obeyed only when doc was created by WinWord 2.x).
    bool        fProtEnabled : 1;   //   when 1, document is protected from edit operations
    bool        fDispFormFieldSel : 1;//   when 1, restrict selections to occur only within form fields
    bool        fRMView : 1;        //   when 1, show revision markings on screen
    bool        fRMPrint : 1;       //   when 1, print revision marks when document is printed
    bool        fWriteReservation : 1;
    bool        fLockRev : 1;       //   when 1, the current revision marking state is locked
    bool        fEmbedFonts : 1;    //   when 1, document contains embedded True Type fonts
    //    compatibility options
    bool       copts_fNoTabForInd : 1;          //    when 1, don't add automatic tab stops for hanging indent
    bool       copts_fNoSpaceRaiseLower : 1;        //    when 1, don't add extra space for raised or lowered characters
    bool       copts_fSupressSpbfAfterPgBrk : 1;    // when 1, suppress the paragraph Space Before and Space After options after a page break
    bool       copts_fWrapTrailSpaces : 1;      //    when 1, wrap trailing spaces at the end of a line to the next line
    bool       copts_fMapPrintTextColor : 1;        //    when 1, print colors as black on non-color printers
    bool       copts_fNoColumnBalance : 1;      //    when 1, don't balance columns for Continuous Section starts
    bool       copts_fConvMailMergeEsc : 1;
    bool       copts_fSupressTopSpacing : 1;        //    when 1, suppress extra line spacing at top of page
    bool       copts_fOrigWordTableRules : 1;   //    when 1, combine table borders like Word 5.x for the Macintosh
    bool       copts_fTransparentMetafiles : 1; //    when 1, don't blank area between metafile pictures
    bool       copts_fShowBreaksInFrames : 1;   //    when 1, show hard page or column breaks in frames
    bool       copts_fSwapBordersFacingPgs : 1; //    when 1, swap left and right pages on odd facing pages
    bool       copts_fExpShRtn : 1;             //    when 1, expand character spaces on the line ending SHIFT+RETURN  // #i56856#

    sal_Int16  dxaTab;              //      720 twips - default tab width
    sal_uInt16 wSpare;
    sal_uInt16 dxaHotZ;             //      width of hyphenation hot zone measured in twips
    sal_uInt16 cConsecHypLim;       //      number of lines allowed to have consecutive hyphens
    sal_uInt16 wSpare2;             //      reserved
    sal_Int32  dttmCreated;         //      DTTM date and time document was created
    sal_Int32  dttmRevised;         //      DTTM date and time document was last revised
    sal_Int32  dttmLastPrint;       //      DTTM date and time document was last printed
    sal_Int16  nRevision;           //      number of times document has been revised since its creation
    sal_Int32  tmEdited;            //      time document was last edited
    sal_Int32  cWords;              //      count of words tallied by last Word Count execution
    sal_Int32  cCh;                 //      count of characters tallied by last Word Count execution
    sal_Int16  cPg;                 //      count of pages tallied by last Word Count execution
    sal_Int32  cParas;              //      count of paragraphs tallied by last Word Count execution
    sal_uInt16 rncEdn : 2;          //      restart endnote number code: 0 don't restart endnote numbering, 1 section, 2 page
    sal_uInt16 nEdn : 14;           //      beginning endnote number
    sal_uInt16 epc : 2;             //      endnote position code: 0 at end of section, 3 at end of document

    bool       fPrintFormData : 1;  //      only print data inside of form fields
    bool       fSaveFormData : 1;   //      only save document data that is inside of a form field.
    bool       fShadeFormData : 1;  //      shade form fields
    sal_uInt16 : 2;                 //      reserved
    bool       fWCFootnoteEdn : 1;       //      when 1, include footnotes and endnotes in word count
    sal_Int32   cLines;             //      count of lines tallied by last Word Count operation
    sal_Int32   cWordsFootnoteEnd;       //      count of words in footnotes and endnotes tallied by last Word Count operation
    sal_Int32   cChFootnoteEdn;          //      count of characters in footnotes and endnotes tallied by last Word Count operation
    sal_Int16   cPgFootnoteEdn;          //      count of pages in footnotes and endnotes tallied by last Word Count operation
    sal_Int32   cParasFootnoteEdn;       //      count of paragraphs in footnotes and endnotes tallied by last Word Count operation
    sal_Int32   cLinesFootnoteEdn;       //      count of paragraphs in footnotes and endnotes tallied by last Word Count operation
    sal_Int32   lKeyProtDoc;        //      document protection password key, only valid if dop.fProtEnabled, dop.fLockAtn or dop.fLockRev are 1.
    sal_uInt16  wvkSaved : 3;       //      document view kind: 0 Normal view, 1 Outline view, 2 Page View
    sal_uInt16  wScaleSaved : 9;    ///< Specifies the zoom percentage that was in use when the document was saved.
    sal_uInt16  zkSaved : 2;        //      document zoom type: 0 percent, 1 whole/entire page, 2 page width, 3 text width/optimal
    bool        fRotateFontW6 : 1;
    bool        iGutterPos : 1 ;

    // this should be the end for nFib < 103, otherwise the file is broken!

    /*
        for nFib >= 103 it continues:
    */
    bool       fNoTabForInd : 1;                      // see above in compatibility options
    bool       fNoSpaceRaiseLower : 1;                // see above
    bool       fSupressSpbfAfterPageBreak : 1;        // see above
    bool       fWrapTrailSpaces : 1;                  // see above
    bool       fMapPrintTextColor : 1;                // see above
    bool       fNoColumnBalance : 1;                  // see above
    bool       fConvMailMergeEsc : 1;                 // see above
    bool       fSupressTopSpacing : 1;                // see above
    bool       fOrigWordTableRules : 1;               // see above
    bool       fTransparentMetafiles : 1;             // see above
    bool       fShowBreaksInFrames : 1;               // see above
    bool       fSwapBordersFacingPgs : 1;             // see above
    bool       fCompatibilityOptions_Unknown1_13 : 1; // #i78591#
    bool       fExpShRtn : 1;                         // #i78591# and #i56856#
    bool       fCompatibilityOptions_Unknown1_15 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_16 : 1; // #i78591#
    bool       fSuppressTopSpacingMac5 : 1;           // Suppress extra line spacing at top
                                                      // of page like MacWord 5.x
    bool       fTruncDxaExpand : 1;                    // Expand/Condense by whole number of points
    bool       fPrintBodyBeforeHdr : 1;               // Print body text before header/footer
    bool       fNoLeading : 1;                        // Don't add extra spacebetween rows of text
    bool       fCompatibilityOptions_Unknown1_21 : 1; // #i78591#
    bool       fMWSmallCaps : 1;                     // Use larger small caps like MacWord 5.x
    bool       fCompatibilityOptions_Unknown1_23 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_24 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_25 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_26 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_27 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_28 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_29 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_30 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown1_31 : 1; // #i78591#
    bool       fUsePrinterMetrics : 1;                //The magic option

    // this should be the end for nFib <= 105, otherwise the file is broken!

    /*
        for nFib > 105 it continues:
    */
    sal_Int16   adt;                  // Autoformat Document Type:
                                      // 0 for normal.
                                      // 1 for letter, and
                                      // 2 for email.
    WW8DopTypography doptypography;   // see WW8STRUC.HXX
    WW8_DOGRID dogrid;                // see WW8STRUC.HXX
    sal_uInt16 : 1;                   // reserved
    sal_uInt16 lvl : 4;               // Which outline levels are showing in outline view
    sal_uInt16 : 4;                   // reserved
    bool       fHtmlDoc : 1;          // This file is based upon an HTML file
    sal_uInt16 : 1;                   // reserved
    bool       fSnapBorder : 1;       // Snap table and page borders to page border
    bool       fIncludeHeader : 1;    // Place header inside page border
    bool       fIncludeFooter : 1;    // Place footer inside page border
    bool       fForcePageSizePag : 1; // Are we in online view
    bool       fMinFontSizePag : 1;   // Are we auto-promoting fonts to >= hpsZoonFontPag?
    bool       fHaveVersions : 1;     // versioning is turned on
    bool       fAutoVersion : 1;      // autoversioning is enabled
    sal_uInt16 : 14;                  // reserved
    // Skip 12 Bytes here: ASUMI
    sal_Int32 cChWS;
    sal_Int32 cChWSFootnoteEdn;
    sal_Int32 grfDocEvents;
    // Skip 4+30+8 Bytes here
    sal_Int32 cDBC;
    sal_Int32 cDBCFootnoteEdn;
    // Skip 4 Bytes here
    sal_Int16 nfcFootnoteRef;
    sal_Int16 nfcEdnRef;
    sal_Int16 hpsZoonFontPag;
    sal_Int16 dywDispPag;

    bool       fCompatibilityOptions_Unknown2_1 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_2 : 1;  // #i78591#
    bool       fDontUseHTMLAutoSpacing : 1;
    bool       fCompatibilityOptions_Unknown2_4 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_5 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_6 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_7 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_8 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_9 : 1;  // #i78591#
    bool       fCompatibilityOptions_Unknown2_10 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_11 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_12 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_13 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_14 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_15 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_16 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_17 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_18 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_19 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_20 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_21 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_22 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_23 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_24 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_25 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_26 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_27 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_28 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_29 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_30 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_31 : 1; // #i78591#
    bool       fCompatibilityOptions_Unknown2_32 : 1; // #i78591#

    sal_uInt16 fUnknown3 : 15;
    bool       fUseBackGroundInAllmodes : 1;

    bool       fDoNotEmbedSystemFont : 1;
    bool       fWordCompat : 1;
    bool       fLiveRecover : 1;
    bool       fEmbedFactoids : 1;
    bool       fFactoidXML : 1;
    bool       fFactoidAllDone : 1;
    bool       fFolioPrint : 1;
    bool       fReverseFolio : 1;
    sal_uInt16 iTextLineEnding : 3;
    bool       fHideFcc : 1;
    bool       fAcetateShowMarkup : 1;
    bool       fAcetateShowAtn : 1;
    bool       fAcetateShowInsDel : 1;
    bool       fAcetateShowProps : 1;

    bool bUseThaiLineBreakingRules;

    /* Constructor for importing, needs to know the version of word used */
    WW8Dop(SvStream& rSt, sal_Int16 nFib, sal_Int32 nPos, sal_uInt32 nSize);

    /* Constructs default DOP suitable for exporting */
    WW8Dop();
    bool Write(SvStream& rStrm, WW8Fib& rFib) const;

    sal_uInt32 GetCompatibilityOptions() const;
    void SetCompatibilityOptions(sal_uInt32 a32Bit);
    // i#78591#
    sal_uInt32 GetCompatibilityOptions2() const;
    void SetCompatibilityOptions2(sal_uInt32 a32Bit);
};

class WW8PLCF_HdFt
{
private:
    WW8PLCF aPLCF;
    long nTextOfs;
    short nIdxOffset;

public:
    WW8PLCF_HdFt( SvStream* pSt, WW8Fib& rFib, WW8Dop& rDop );
    bool GetTextPos(sal_uInt8 grpfIhdt, sal_uInt8 nWhich, WW8_CP& rStart, WW8_CP& rLen);
    bool GetTextPosExact(short nIdx, WW8_CP& rStart, WW8_CP& rLen);
    void UpdateIndex( sal_uInt8 grpfIhdt );
};

void SwapQuotesInField(OUString &rFormat);

Word2CHPX ReadWord2Chpx(SvStream &rSt, sal_Size nOffset, sal_uInt8 nSize);
std::vector<sal_uInt8> ChpxToSprms(const Word2CHPX &rChpx);

bool checkSeek(SvStream &rSt, sal_uInt32 nOffset)
    SAL_WARN_UNUSED_RESULT;

bool checkRead(SvStream &rSt, void *pDest, sal_uInt32 nLength)
    SAL_WARN_UNUSED_RESULT;

//MS has a (slightly) inaccurate view of how many twips
//are in the default letter size of a page
const sal_uInt16 lLetterWidth = 12242;
const sal_uInt16 lLetterHeight = 15842;

#ifdef OSL_BIGENDIAN
void swapEndian(sal_Unicode *pString);
#endif

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
