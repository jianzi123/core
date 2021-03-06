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
#ifndef INCLUDED_SVX_SRCHDLG_HXX
#define INCLUDED_SVX_SRCHDLG_HXX

#include <svtools/stdctrl.hxx>
#include <vcl/combobox.hxx>
#include <vcl/edit.hxx>
#include <vcl/button.hxx>
#include <vcl/layout.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/group.hxx>
#include <vcl/fixed.hxx>
#include <vcl/dialog.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/basedlgs.hxx>
#include <svtools/svmedit.hxx>
#include <svl/srchdefs.hxx>
#include <svx/svxdllapi.h>
#include <vector>

class SvxSearchItem;
class SfxStyleSheetBasePool;
class SvxJSearchOptionsPage;
class SvxSearchController;

struct SearchDlg_Impl;

struct SearchAttrItem
{
    sal_uInt16          nSlot;
    SfxPoolItem*    pItem;
};

// class SearchAttrItemList ----------------------------------------------

typedef std::vector<SearchAttrItem> SrchAttrItemList;

class SVX_DLLPUBLIC SearchAttrItemList : private SrchAttrItemList
{
public:
    SearchAttrItemList() {}
    SearchAttrItemList( const SearchAttrItemList& rList );
    ~SearchAttrItemList();

    void            Put( const SfxItemSet& rSet );
    SfxItemSet&     Get( SfxItemSet& rSet );
    void            Clear();
    sal_uInt16      Count() const { return SrchAttrItemList::size(); }
    SearchAttrItem& operator[](sal_uInt16 nPos)
                        { return SrchAttrItemList::operator[]( nPos ); }
    SearchAttrItem& GetObject( sal_uInt16 nPos )
                        { return SrchAttrItemList::operator[]( nPos ); }

    // the pointer to the item is not being copied, so don't delete
    void Insert( const SearchAttrItem& rItem )
        { SrchAttrItemList::push_back( rItem ); }
    // deletes the pointer to the items
    void Remove(size_t nPos, size_t nLen = 1);
};


// class SvxSearchDialogWrapper ------------------------------------------

enum SearchLabel
{
    SL_Empty,
    SL_End,
    SL_Start,
    SL_EndSheet,
    SL_NotFound
};

class SvxSearchDialog;
class SVX_DLLPUBLIC SvxSearchDialogWrapper : public SfxChildWindow
{
    VclPtr<SvxSearchDialog> dialog;
public:
    SvxSearchDialogWrapper( vcl::Window*pParent, sal_uInt16 nId,
                            SfxBindings* pBindings, SfxChildWinInfo* pInfo );

    virtual ~SvxSearchDialogWrapper ();
    SvxSearchDialog *getDialog () { return dialog;}
    static void SetSearchLabel(const SearchLabel& rSL);
    SFX_DECL_CHILDWINDOW_WITHID(SvxSearchDialogWrapper);
};

// class SvxSearchDialog -------------------------------------------------
/*
    [Description]
    In this modeless dialog the attributes for a search are configured
    and a search is started from it. Several search types
    (search, search all, replace, replace all) are possible.

    [Items]
    <SvxSearchItem><SID_ATTR_SEARCH>
*/

class SvxSearchDialog : public SfxModelessDialog
{
friend class SvxSearchController;
friend class SvxSearchDialogWrapper;
friend class SvxJSearchOptionsDialog;

public:
    SvxSearchDialog( vcl::Window* pParent, SfxChildWindow* pChildWin, SfxBindings& rBind );
    virtual ~SvxSearchDialog();
    virtual void dispose() SAL_OVERRIDE;

    virtual bool    Close() SAL_OVERRIDE;

    // Window
    virtual void    Activate() SAL_OVERRIDE;

    const SearchAttrItemList*   GetSearchItemList() const
                                    { return pSearchList; }
    const SearchAttrItemList*   GetReplaceItemList() const
                                    { return pReplaceList; }

    sal_Int32       GetTransliterationFlags() const;

    void SetDocWin( vcl::Window* pDocWin ) { mpDocWin = pDocWin; }
    vcl::Window* GetDocWin() { return mpDocWin; }
    void SetSrchFlag( bool bSuccess = false ) { mbSuccess = bSuccess; }
    bool GetSrchFlag() { return mbSuccess; }
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindowPeer >
        GetComponentInterface( bool bCreate ) SAL_OVERRIDE;

    void            SetSaveToModule(bool b);

    void SetSearchLabel(const OUString& rStr) { m_pSearchLabel->SetText(rStr); }

private:
    VclPtr<vcl::Window>         mpDocWin;
    bool            mbSuccess;

    VclPtr<VclFrame>       m_pSearchFrame;
    VclPtr<ComboBox>       m_pSearchLB;
    VclPtr<ListBox>        m_pSearchTmplLB;
    VclPtr<FixedText>      m_pSearchAttrText;
    VclPtr<FixedText>      m_pSearchLabel;

    VclPtr<VclFrame>       m_pReplaceFrame;
    VclPtr<ComboBox>       m_pReplaceLB;
    VclPtr<ListBox>        m_pReplaceTmplLB;
    VclPtr<FixedText>      m_pReplaceAttrText;

    VclPtr<PushButton>    m_pSearchBtn;
    VclPtr<PushButton>     m_pSearchAllBtn;
    VclPtr<PushButton>     m_pReplaceBtn;
    VclPtr<PushButton>     m_pReplaceAllBtn;

    VclPtr<VclFrame>       m_pComponentFrame;
    VclPtr<PushButton>     m_pSearchComponent1PB;
    VclPtr<PushButton>     m_pSearchComponent2PB;

    VclPtr<CheckBox>       m_pMatchCaseCB;
    VclPtr<CheckBox>       m_pWordBtn;

    VclPtr<PushButton>     m_pCloseBtn;
    VclPtr<CheckBox>       m_pIgnoreDiacritics;
    VclPtr<CheckBox>       m_pIgnoreKashida;
    VclPtr<CheckBox>       m_pSelectionBtn;
    VclPtr<CheckBox>       m_pBackwardsBtn;
    VclPtr<CheckBox>       m_pRegExpBtn;
    VclPtr<CheckBox>       m_pSimilarityBox;
    VclPtr<PushButton>     m_pSimilarityBtn;
    VclPtr<CheckBox>       m_pLayoutBtn;
    VclPtr<CheckBox>       m_pNotesBtn;
    VclPtr<CheckBox>       m_pJapMatchFullHalfWidthCB;
    VclPtr<CheckBox>       m_pJapOptionsCB;
    VclPtr<PushButton>     m_pJapOptionsBtn;

    VclPtr<PushButton>     m_pAttributeBtn;
    VclPtr<PushButton>     m_pFormatBtn;
    VclPtr<PushButton>     m_pNoFormatBtn;

    VclPtr<VclContainer>   m_pCalcGrid;
    VclPtr<FixedText>      m_pCalcSearchInFT;
    VclPtr<ListBox>        m_pCalcSearchInLB;
    VclPtr<FixedText>      m_pCalcSearchDirFT;
    VclPtr<RadioButton>    m_pRowsBtn;
    VclPtr<RadioButton>    m_pColumnsBtn;
    VclPtr<CheckBox>       m_pAllSheetsCB;

    SfxBindings&    rBindings;
    bool            bWriter;
    bool            bSearch;
    bool            bFormat;
    SearchOptionFlags  nOptions;
    bool            bSet;
    bool            bReadOnly;
    bool            bConstruct;
    sal_uIntPtr         nModifyFlag;
    OUString        aStylesStr;
    OUString        aLayoutStr;
    OUString        aLayoutWriterStr;
    OUString        aLayoutCalcStr;
    OUString        aCalcStr;

    std::vector<OUString> aSearchStrings;
    std::vector<OUString> aReplaceStrings;

    SearchDlg_Impl*         pImpl;
    SearchAttrItemList*     pSearchList;
    SearchAttrItemList*     pReplaceList;
    SvxSearchItem*          pSearchItem;

    SvxSearchController*    pSearchController;
    SvxSearchController*    pOptionsController;
    SvxSearchController*    pFamilyController;
    SvxSearchController*    pSearchSetController;
    SvxSearchController*    pReplaceSetController;

    mutable sal_Int32           nTransliterationFlags;

    DECL_LINK( ModifyHdl_Impl, ComboBox* pEdit );
    DECL_LINK( FlagHdl_Impl, Control* pCtrl );
    DECL_LINK( CommandHdl_Impl, Button* pBtn );
    DECL_LINK(TemplateHdl_Impl, void *);
    DECL_LINK( FocusHdl_Impl, Control* );
    DECL_LINK(LoseFocusHdl_Impl, void *);
    DECL_LINK(FormatHdl_Impl, void *);
    DECL_LINK(NoFormatHdl_Impl, void *);
    DECL_LINK(AttributeHdl_Impl, void *);
    DECL_LINK_TYPED( TimeoutHdl_Impl, Timer*, void );

    void            Construct_Impl();
    void            InitControls_Impl();
    void            ShowOptionalControls_Impl();
    void            Init_Impl( bool bHasItemSet );
    void            InitAttrList_Impl( const SfxItemSet* pSSet,
                                       const SfxItemSet* pRSet );
    void            Remember_Impl( const OUString &rStr, bool bSearch );
    void            PaintAttrText_Impl();
    OUString&       BuildAttrText_Impl( OUString& rStr, bool bSrchFlag ) const;

    void            TemplatesChanged_Impl( SfxStyleSheetBasePool& rPool );
    void            EnableControls_Impl( const SearchOptionFlags nFlags );
    void            EnableControl_Impl( Control* pCtrl );
    void            SetItem_Impl( const SvxSearchItem* pItem );

    void            SetModifyFlag_Impl( const Control* pCtrl );
    void            SaveToModule_Impl();

    void            ApplyTransliterationFlags_Impl( sal_Int32 nSettings );
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
