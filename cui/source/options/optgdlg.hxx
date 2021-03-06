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
#ifndef INCLUDED_CUI_SOURCE_OPTIONS_OPTGDLG_HXX
#define INCLUDED_CUI_SOURCE_OPTIONS_OPTGDLG_HXX
#include <vcl/lstbox.hxx>
#include <vcl/group.hxx>
#include <vcl/field.hxx>
#include <vcl/fixed.hxx>
#include <sfx2/tabdlg.hxx>
#include <svx/langbox.hxx>

#include <boost/scoped_ptr.hpp>

class CanvasSettings;
class SvtOptionsDrawinglayer;

namespace vcl {
    class IconThemeInfo;
}

namespace svt {
    class OpenGLCfg;
}

class OfaMiscTabPage : public SfxTabPage
{
    using TabPage::DeactivatePage;
private:
    VclPtr<CheckBox>     m_pExtHelpCB;

    VclPtr<FixedImage>   m_pFileDlgROImage;
    VclPtr<CheckBox>     m_pFileDlgCB;

    VclPtr<CheckBox>     m_pPrintDlgCB;

    VclPtr<CheckBox>     m_pDocStatusCB;
    VclPtr<CheckBox>     m_pSaveAlwaysCB;

    VclPtr<VclContainer> m_pYearFrame;
    VclPtr<NumericField> m_pYearValueField;
    VclPtr<FixedText>    m_pToYearFT;

    OUString      m_aStrDateInfo;

    VclPtr<CheckBox>     m_pCollectUsageInfo;

    DECL_LINK( TwoFigureHdl, NumericField* );
    DECL_LINK( TwoFigureConfigHdl, NumericField* );
#ifdef WNT
    DECL_LINK( OnFileDlgToggled, void* );
#endif
protected:
    virtual sfxpg       DeactivatePage( SfxItemSet* pSet = NULL ) SAL_OVERRIDE;

public:
    OfaMiscTabPage( vcl::Window* pParent, const SfxItemSet& rSet );
    virtual ~OfaMiscTabPage();
    virtual void dispose() SAL_OVERRIDE;

    static VclPtr<SfxTabPage>  Create( vcl::Window* pParent, const SfxItemSet* rAttrSet );

    virtual bool        FillItemSet( SfxItemSet* rSet ) SAL_OVERRIDE;
    virtual void        Reset( const SfxItemSet* rSet ) SAL_OVERRIDE;
};

class SvtTabAppearanceCfg;

class OfaViewTabPage : public SfxTabPage
{
private:
    VclPtr<MetricField>    m_pWindowSizeMF;
    VclPtr<ListBox>        m_pIconSizeLB;
    VclPtr<ListBox>        m_pIconStyleLB;

    VclPtr<CheckBox>       m_pFontAntiAliasing;
    VclPtr<FixedText>      m_pAAPointLimitLabel;
    VclPtr<MetricField>    m_pAAPointLimit;

    VclPtr<ListBox>        m_pMenuIconsLB;

    VclPtr<CheckBox>       m_pFontShowCB;

    VclPtr<CheckBox>       m_pUseHardwareAccell;
    VclPtr<CheckBox>       m_pUseAntiAliase;
    VclPtr<CheckBox>       m_pUseOpenGL;
    VclPtr<CheckBox>       m_pForceOpenGL;

    VclPtr<ListBox>        m_pMousePosLB;
    VclPtr<ListBox>        m_pMouseMiddleLB;

    sal_uInt16      nSizeLB_InitialSelection;
    sal_uInt16      nStyleLB_InitialSelection;

    SvtTabAppearanceCfg*    pAppearanceCfg;
    CanvasSettings*         pCanvasSettings;
    SvtOptionsDrawinglayer* mpDrawinglayerOpt;
    boost::scoped_ptr<svt::OpenGLCfg> mpOpenGLConfig;

    std::vector<vcl::IconThemeInfo> mInstalledIconThemes;

#if defined( UNX )
    DECL_LINK( OnAntialiasingToggled, void* );
#endif

public:
    OfaViewTabPage( vcl::Window* pParent, const SfxItemSet& rSet );
    virtual ~OfaViewTabPage();
    virtual void dispose() SAL_OVERRIDE;

    static VclPtr<SfxTabPage>  Create( vcl::Window* pParent, const SfxItemSet* rAttrSet );

    virtual bool        FillItemSet( SfxItemSet* rSet ) SAL_OVERRIDE;
    virtual void        Reset( const SfxItemSet* rSet ) SAL_OVERRIDE;
};

struct LanguageConfig_Impl;

class OfaLanguagesTabPage : public SfxTabPage
{
    VclPtr<ListBox>        m_pUserInterfaceLB;
    VclPtr<FixedText>      m_pLocaleSettingFT;
    VclPtr<SvxLanguageBox> m_pLocaleSettingLB;
    VclPtr<CheckBox>       m_pDecimalSeparatorCB;
    VclPtr<FixedText>      m_pCurrencyFT;
    VclPtr<ListBox>        m_pCurrencyLB;
    VclPtr<FixedText>      m_pDatePatternsFT;
    VclPtr<Edit>           m_pDatePatternsED;

    VclPtr<SvxLanguageBox> m_pWesternLanguageLB;
    VclPtr<FixedText>      m_pWesternLanguageFT;
    VclPtr<SvxLanguageBox> m_pAsianLanguageLB;
    VclPtr<SvxLanguageBox> m_pComplexLanguageLB;
    VclPtr<CheckBox>       m_pCurrentDocCB;
    VclPtr<CheckBox>       m_pAsianSupportCB;
    VclPtr<CheckBox>       m_pCTLSupportCB;
    VclPtr<CheckBox>       m_pIgnoreLanguageChangeCB;

    bool        m_bOldAsian;
    bool        m_bOldCtl;
    LanguageConfig_Impl*    pLangConfig;

    OUString        m_sUserLocaleValue;
    OUString        m_sSystemDefaultString;

    bool            m_bDatePatternsValid;

    DECL_LINK(  SupportHdl, CheckBox* ) ;
    DECL_LINK(  LocaleSettingHdl, SvxLanguageBox* ) ;
    DECL_LINK(  DatePatternsHdl, Edit* ) ;

public:
    OfaLanguagesTabPage( vcl::Window* pParent, const SfxItemSet& rSet );
    virtual ~OfaLanguagesTabPage();
    virtual void dispose() SAL_OVERRIDE;

    static VclPtr<SfxTabPage>  Create( vcl::Window* pParent, const SfxItemSet* rAttrSet );

    virtual bool        FillItemSet( SfxItemSet* rSet ) SAL_OVERRIDE;
    virtual void        Reset( const SfxItemSet* rSet ) SAL_OVERRIDE;
};

#endif // INCLUDED_CUI_SOURCE_OPTIONS_OPTGDLG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
