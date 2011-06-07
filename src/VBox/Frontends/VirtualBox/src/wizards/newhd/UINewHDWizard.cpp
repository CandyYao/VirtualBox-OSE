/* $Id$ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UINewHDWizard class implementation
 */

/*
 * Copyright (C) 2006-2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Global includes: */
#include <QCheckBox>
#include <QRadioButton>
#include <QRegExpValidator>

/* Local includes: */
#include "VBoxGlobal.h"
#include "VBoxProblemReporter.h"
#include "QIFileDialog.h"
#include "UIIconPool.h"
#include "UINewHDWizard.h"
#include "iprt/path.h"

/* Class to manage page variants: */
class UIExclusivenessManager : public QObject
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIExclusivenessManager(QWidget *pParent) : QObject(pParent) {}

    /* Wrapper for adding different children: */
    void addWidget(QWidget *pWidget, const QVariant &data)
    {
        /* Add radio-button: */
        if (QRadioButton *pRadioButton = qobject_cast<QRadioButton*>(pWidget))
            addRadioButton(pRadioButton, data);
        /* Add check-box: */
        if (QCheckBox *pCheckBox = qobject_cast<QCheckBox*>(pWidget))
            addCheckBox(pCheckBox, data);
    }

    /* Wrapper for different children data: */
    QVariant data(QWidget *pWidget) const
    {
        /* Return data for radio-button: */
        if (QRadioButton *pRadioButton = qobject_cast<QRadioButton*>(pWidget))
            return dataOfRadioButton(pRadioButton);
        /* Return data for check-box: */
        if (QCheckBox *pCheckBox = qobject_cast<QCheckBox*>(pWidget))
            return dataOfCheckBox(pCheckBox);
        /* Return empty data: */
        return QVariant();
    }

    void reset()
    {
        /* Make sure all radio-buttons are unchecked: */
        QList<QRadioButton*> radioButtons = m_radioButtons.keys();
        for (int i = 0; i < radioButtons.size(); ++i)
        {
            if (radioButtons[i]->isChecked())
            {
                radioButtons[i]->setAutoExclusive(false);
                radioButtons[i]->setChecked(false);
                radioButtons[i]->setAutoExclusive(true);
            }
        }
        /* Make sure all check-boxes are unchecked: */
        QList<QCheckBox*> checkBoxes = m_checkBoxes.keys();
        for (int i = 0; i < checkBoxes.size(); ++i)
        {
            if (checkBoxes[i]->isChecked())
                checkBoxes[i]->setChecked(false);
        }
    }

signals:

    void sigNotifyAboutStateChange(QVariant exclusiveData, QList<QVariant> optionsData);

private slots:

    void sltRadioButtonToggled()
    {
        recalculateState();
    }

    void sltCheckBoxToggled()
    {
        recalculateState();
    }

private:

    void addRadioButton(QRadioButton *pRadioButton, const QVariant &exclusiveData)
    {
        /* Setup the connections: */
        connect(pRadioButton, SIGNAL(toggled(bool)), this, SLOT(sltRadioButtonToggled()));
        /* Add radio-button into corresponding list: */
        m_radioButtons.insert(pRadioButton, exclusiveData);
    }

    void addCheckBox(QCheckBox *pCheckBox, const QVariant &optionData)
    {
        /* Setup the connections: */
        connect(pCheckBox, SIGNAL(toggled(bool)), this, SLOT(sltCheckBoxToggled()));
        /* Add check-box into corresponding list: */
        m_checkBoxes.insert(pCheckBox, optionData);
    }

    QVariant dataOfRadioButton(QRadioButton *pRadioButton) const
    {
        /* Return radio-button data if present: */
        if (m_radioButtons.contains(pRadioButton))
            return m_radioButtons[pRadioButton];
        /* Return empty data: */
        return QVariant();
    }

    QVariant dataOfCheckBox(QCheckBox *pCheckBox) const
    {
        /* Return check-box data if present: */
        if (m_checkBoxes.contains(pCheckBox))
            return m_checkBoxes[pCheckBox];
        /* Return empty data: */
        return QVariant();
    }

    void recalculateState()
    {
        /* Prepare current state: */
        QList<bool> currentState;
        /* Get the list of radio-buttons: */
        QList<QRadioButton*> radioButtons = m_radioButtons.keys();
        /* Get the list of check-boxes: */
        QList<QCheckBox*> checkBoxes = m_checkBoxes.keys();

        /* Calculate current state: */
        for (int i = 0; i < radioButtons.size(); ++i)
            currentState << radioButtons[i]->isChecked();
        for (int i = 0; i < checkBoxes.size(); ++i)
            currentState << checkBoxes[i]->isChecked();

        /* Check if state was changed: */
        if (m_state == currentState)
            return;

        /* Search for exclusive data: */
        QVariant exclusiveData;
        for (int i = 0; i < radioButtons.size(); ++i)
        {
            if (radioButtons[i]->isChecked())
            {
                exclusiveData = m_radioButtons[radioButtons[i]];
                break;
            }
        }

        /* Search for options data: */
        QList<QVariant> optionsData;
        for (int i = 0; i < checkBoxes.size(); ++i)
        {
            if (checkBoxes[i]->isChecked())
                optionsData << m_checkBoxes[checkBoxes[i]];
        }

        /* Notify listeners about state-change: */
        emit sigNotifyAboutStateChange(exclusiveData, optionsData);
    }

    QMap<QRadioButton*, QVariant> m_radioButtons;
    QMap<QCheckBox*, QVariant> m_checkBoxes;
    QList<bool> m_state;
};

UINewHDWizard::UINewHDWizard(QWidget *pParent, const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize)
    : QIWizard(pParent)
{
    /* Create & add pages: */
    addPage(new UINewHDWizardPageWelcome);
    addPage(new UINewHDWizardPageFormat);
    addPage(new UINewHDWizardPageVariant);
    addPage(new UINewHDWizardPageOptions(strDefaultName, strDefaultPath, uDefaultSize));
    addPage(new UINewHDWizardPageSummary);

    /* Translate wizard: */
    retranslateUi();

    /* Translate wizard pages: */
    retranslateAllPages();

    /* Resize wizard to 'golden ratio': */
    resizeToGoldenRatio();

#ifdef Q_WS_MAC
    /* Assign background image: */
    assignBackground(":/vmw_new_harddisk_bg.png");
#else /* Q_WS_MAC */
    /* Assign watermark: */
    assignWatermark(":/vmw_new_harddisk.png");
#endif /* Q_WS_MAC */
}

CMedium UINewHDWizard::hardDisk() const
{
    /* Return 'hardDisk' field value from 'summary' page: */
    return field("hardDisk").value<CMedium>();
}

void UINewHDWizard::retranslateUi()
{
    /* Assign wizard title: */
    setWindowTitle(tr("Create New Virtual Disk"));
}

UINewHDWizardPageWelcome::UINewHDWizardPageWelcome()
{
    /* Decorate page: */
    Ui::UINewHDWizardPageWelcome::setupUi(this);
}

void UINewHDWizardPageWelcome::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UINewHDWizardPageWelcome::retranslateUi(this);

    /* Set 'welcome' page title: */
    setTitle(tr("Welcome to the virtual disk creation wizard"));

    /* Append page text with common part: */
    QString strCommonPart = QString("<p>%1</p>").arg(standardHelpText());
    m_pLabel->setText(m_pLabel->text() + strCommonPart);
}

void UINewHDWizardPageWelcome::initializePage()
{
    /* Retranslate page: */
    retranslateUi();
}

UINewHDWizardPageFormat::UINewHDWizardPageFormat()
    : m_pExclusivenessManager(0)
    , m_pDefaultButton(0)
{
    /* Decorate page: */
    Ui::UINewHDWizardPageFormat::setupUi(this);

    /* Register extended metatypes: */
    qRegisterMetaType<CMediumFormat>();

    /* Register 'mediumFormat' field: */
    registerField("mediumFormat", this, "mediumFormat");

    /* Create exclusiveness manager: */
    m_pExclusivenessManager = new UIExclusivenessManager(this);
    connect(m_pExclusivenessManager, SIGNAL(sigNotifyAboutStateChange(QVariant, QList<QVariant>)), this, SLOT(sltUpdateFormat(QVariant)));

    /* Enumerate supportable formats: */
    CSystemProperties systemProperties = vboxGlobal().virtualBox().GetSystemProperties();
    QVector<CMediumFormat> mediumFormats = systemProperties.GetMediumFormats();
    for (int iMediumFormatIndex = 0; iMediumFormatIndex < mediumFormats.size(); ++iMediumFormatIndex)
    {
        /* Get iterated medium format: */
        CMediumFormat mediumFormat = mediumFormats[iMediumFormatIndex];

        /* Check that medium format supports creation: */
        ULONG uFormatCapabilities = mediumFormat.GetCapabilities();
        if (!(uFormatCapabilities & MediumFormatCapabilities_CreateFixed ||
              uFormatCapabilities & MediumFormatCapabilities_CreateDynamic))
            continue;

        /* Check that medium format supports creation of hard-disks: */
        QVector<QString> fileExtensions;
        QVector<KDeviceType> deviceTypes;
        mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
        if (!deviceTypes.contains(KDeviceType_HardDisk))
            continue;

        /* Create corresponding radio-button: */
        QRadioButton *pFormatButton = new QRadioButton(m_pFormatContainer);
        m_pExclusivenessManager->addWidget(pFormatButton, QVariant::fromValue(mediumFormat));
        if (mediumFormat.GetName().toLower() == "vdi")
        {
            m_pFormatsLayout->insertWidget(0, pFormatButton);
            m_pDefaultButton = pFormatButton;
        }
        else
            m_pFormatsLayout->addWidget(pFormatButton);
    }
}

/* static */
QString UINewHDWizardPageFormat::fullFormatName(const QString &strBaseFormatName)
{
    if (strBaseFormatName == "VDI")
        return tr("&VDI (VirtualBox Disk Image)");
    else if (strBaseFormatName == "VMDK")
        return tr("V&MDK (Virtual Machine Disk)");
    else if (strBaseFormatName == "VHD")
        return tr("V&HD (Virtual Hard Disk)");
    return strBaseFormatName;
}

void UINewHDWizardPageFormat::sltUpdateFormat(QVariant formatData)
{
    /* Get medium format: */
    CMediumFormat mediumFormat = formatData.value<CMediumFormat>();

    /* Check if medium format was changed: */
    if (m_mediumFormat == mediumFormat)
        return;

    /* Update medium format: */
    m_mediumFormat = mediumFormat;

    /* Notify wizard sub-system about complete status changed: */
    emit completeChanged();
}

void UINewHDWizardPageFormat::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UINewHDWizardPageFormat::retranslateUi(this);

    /* Set 'format' page title: */
    setTitle(tr("Virtual disk file type"));

    /* Translate 'format' buttons: */
    QList<QRadioButton*> formatButtons = findChildren<QRadioButton*>();
    for (int i = 0; i < formatButtons.size(); ++i)
    {
        QRadioButton *pFormatButton = formatButtons[i];
        CMediumFormat mediumFormat = m_pExclusivenessManager->data(pFormatButton).value<CMediumFormat>();
        pFormatButton->setText(fullFormatName(mediumFormat.GetName()));
    }
}

void UINewHDWizardPageFormat::initializePage()
{
    /* Retranslate page: */
    retranslateUi();

    /* Make sure first of buttons (default) is checked: */
    m_pDefaultButton->setChecked(true);
    m_pDefaultButton->setFocus();
}

void UINewHDWizardPageFormat::cleanupPage()
{
    /* Reset exclusiveness manager: */
    m_pExclusivenessManager->reset();
    /* Call for base-class: */
    UINewHDWizardPage::cleanupPage();
}

bool UINewHDWizardPageFormat::isComplete() const
{
    return !m_mediumFormat.isNull();
}

UINewHDWizardPageVariant::UINewHDWizardPageVariant()
    : m_pExclusivenessManager(0)
    , m_pDynamicalButton(0), m_pFixedButton(0), m_pSplitBox(0)
    , m_uMediumVariant(KMediumVariant_Max)
{
    /* Decorate page: */
    Ui::UINewHDWizardPageVariant::setupUi(this);

    /* Register 'mediumVariant' field: */
    registerField("mediumVariant", this, "mediumVariant");

    /* Unfortuanelly, KMediumVariant is very messy,
     * so we can't enumerate it to make sure GUI will not hard-code its values,
     * we can only use hard-coded values that we need: */

    /* Create exclusiveness manager: */
    m_pExclusivenessManager = new UIExclusivenessManager(this);
    connect(m_pExclusivenessManager, SIGNAL(sigNotifyAboutStateChange(QVariant, QList<QVariant>)), this, SLOT(sltUpdateVariant(QVariant, QList<QVariant>)));

    /* Create 'dynamical' (standard) variant radio-button: */
    m_pDynamicalButton = new QRadioButton(m_pVariantContainer);
    m_pVariantsLayout->addWidget(m_pDynamicalButton);
    m_pExclusivenessManager->addWidget(m_pDynamicalButton, QVariant((qulonglong)KMediumVariant_Standard));

    /* Create 'fixed' variant radio-button: */
    m_pFixedButton = new QRadioButton(m_pVariantContainer);
    m_pVariantsLayout->addWidget(m_pFixedButton);
    m_pExclusivenessManager->addWidget(m_pFixedButton, QVariant((qulonglong)(KMediumVariant_Standard | KMediumVariant_Fixed)));

    /* Create '2GByte' variant check-box: */
    m_pSplitBox = new QCheckBox(m_pVariantContainer);
    m_pVariantsLayout->addWidget(m_pSplitBox);
    m_pExclusivenessManager->addWidget(m_pSplitBox, QVariant((qulonglong)(KMediumVariant_VmdkSplit2G)));
}

/* static */
QString UINewHDWizardPageVariant::variantToString(qulonglong uMediumVariant)
{
    switch (uMediumVariant)
    {
        case (qulonglong)KMediumVariant_Standard:
            return tr("Dynamically expanding storage");
        case (qulonglong)(KMediumVariant_Standard | KMediumVariant_Fixed):
            return tr("Fixed size storage");
        case (qulonglong)(KMediumVariant_Standard | KMediumVariant_VmdkSplit2G):
            return tr("Dynamically expanding storage split into files of less than 2GB");
        case (qulonglong)(KMediumVariant_Standard | KMediumVariant_Fixed | KMediumVariant_VmdkSplit2G):
            return tr("Fixed size storage split into files of less than 2GB");
        default:
            break;
    }
    return QString();
}

void UINewHDWizardPageVariant::sltUpdateVariant(QVariant exclusiveData, QList<QVariant> optionsData)
{
    /* Gather new data: */
    qulonglong uMediumVariant = exclusiveData.isNull() ? (qulonglong)KMediumVariant_Max : exclusiveData.toULongLong();
    for (int i = 0; i < optionsData.size(); ++i)
        uMediumVariant |= optionsData[i].toULongLong();

    /* Check if medium variant was changed: */
    if (m_uMediumVariant == uMediumVariant)
        return;

    /* Update medium variant: */
    m_uMediumVariant = uMediumVariant;

    /* Notify wizard sub-system about complete status changed: */
    emit completeChanged();
}

void UINewHDWizardPageVariant::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UINewHDWizardPageVariant::retranslateUi(this);

    /* Set 'variant' page title: */
    setTitle(tr("Virtual disk storage details"));

    /* Translate other text: */
    QString strText = m_pLabel->text();
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    if (mediumFormat.isNull() || (mediumFormat.GetCapabilities() & KMediumFormatCapabilities_CreateDynamic))
        strText += tr("<p>A <b>dynamically expanding</b> virtual disk file will only use space on your physical hard disk as it fills up, "
                      "although it will not shrink again automatically when space on it is freed.</p>");
    if (mediumFormat.isNull() || (mediumFormat.GetCapabilities() & KMediumFormatCapabilities_CreateFixed))
        strText += tr("<p>A <b>fixed size</b> virtual disk file may take longer to create on some systems but is often faster to use.</p>");
    if (mediumFormat.isNull() || (mediumFormat.GetCapabilities() & KMediumFormatCapabilities_CreateSplit2G))
        strText += tr("<p>You can also choose to <b>split</b> the virtual disk into several files of up to two gigabytes each. "
                      "This is mainly useful if you wish to store the virtual machine on removable USB devices or old systems, "
                      "some of which cannot handle very large files.");
    m_pLabel->setText(strText);

    /* Translate buttons: */
    m_pDynamicalButton->setText(tr("&Dynamically expanding"));
    m_pFixedButton->setText(tr("&Fixed size"));
    m_pSplitBox->setText(tr("&Split into files of less than 2GB"));
}

void UINewHDWizardPageVariant::initializePage()
{
    /* Retranslate page: */
    retranslateUi();

    /* Setup visibility: */
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    ULONG uCapabilities = mediumFormat.GetCapabilities();
    m_pDynamicalButton->setVisible(uCapabilities & KMediumFormatCapabilities_CreateDynamic);
    m_pFixedButton->setVisible(uCapabilities & KMediumFormatCapabilities_CreateFixed);
    m_pSplitBox->setVisible(uCapabilities & KMediumFormatCapabilities_CreateSplit2G);
    /* Make sure first of buttons (default) is checked if visible: */
    if (!m_pDynamicalButton->isHidden())
    {
        m_pDynamicalButton->setChecked(true);
        m_pDynamicalButton->setFocus();
    }
}

void UINewHDWizardPageVariant::cleanupPage()
{
    /* Reset exclusiveness manager: */
    m_pExclusivenessManager->reset();
    /* Call for base-class: */
    UINewHDWizardPage::cleanupPage();
}

bool UINewHDWizardPageVariant::isComplete() const
{
    return m_uMediumVariant != KMediumVariant_Max;
}

UINewHDWizardPageOptions::UINewHDWizardPageOptions(const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize)
    : m_strDefaultPath(strDefaultPath)
    , m_strMediumName(strDefaultName.isEmpty() ? QString("NewHardDisk1") : strDefaultName)
    , m_uMediumSize(uDefaultSize == 0 ? (qulonglong)_1G * 2 : uDefaultSize)
    , m_uMediumSizeMin(_4M)
    , m_uMediumSizeMax(vboxGlobal().virtualBox().GetSystemProperties().GetInfoVDSize())
    , m_iSliderScale(0)
{
    /* Decorate page: */
    Ui::UINewHDWizardPageOptions::setupUi(this);

    /* Register 'mediumName', 'mediumPath', 'mediumSize' fields: */
    registerField("mediumName", this, "mediumName");
    registerField("mediumPath", this, "mediumPath");
    registerField("mediumSize", this, "mediumSize");

    /* Detect how many steps to recognize between adjacent powers of 2
     * to ensure that the last slider step is exactly m_uMediumSizeMax: */
    int iPower = log2i(m_uMediumSizeMax);
    qulonglong uTickMB = qulonglong (1) << iPower;
    if (uTickMB < m_uMediumSizeMax)
    {
        qulonglong uTickMBNext = qulonglong (1) << (iPower + 1);
        qulonglong uGap = uTickMBNext - m_uMediumSizeMax;
        m_iSliderScale = (int)((uTickMBNext - uTickMB) / uGap);
    }
    m_iSliderScale = qMax(m_iSliderScale, 8);

    /* Setup size-editor field: */
    m_pSizeEditor->setFixedWidthByText("88888.88 MB");
    m_pSizeEditor->setAlignment(Qt::AlignRight);
    m_pSizeEditor->setValidator(new QRegExpValidator(QRegExp(vboxGlobal().sizeRegexp()), this));

    /* Setup size-slider: */
    m_pSizeSlider->setFocusPolicy(Qt::StrongFocus);
    m_pSizeSlider->setPageStep(m_iSliderScale);
    m_pSizeSlider->setSingleStep(m_iSliderScale / 8);
    m_pSizeSlider->setTickInterval(0);
    m_pSizeSlider->setMinimum(sizeMBToSlider(m_uMediumSizeMin, m_iSliderScale));
    m_pSizeSlider->setMaximum(sizeMBToSlider(m_uMediumSizeMax, m_iSliderScale));
    m_pSizeMin->setText(vboxGlobal().formatSize(m_uMediumSizeMin));
    m_pSizeMax->setText(vboxGlobal().formatSize(m_uMediumSizeMax));

    /* Attach button icon: */
    m_pLocationSelector->setIcon(UIIconPool::iconSet(":/select_file_16px.png", "select_file_dis_16px.png"));

    /* Setup page connections: */
    connect(m_pLocationEditor, SIGNAL(textChanged(const QString &)), this, SLOT(sltLocationEditorTextChanged(const QString &)));
    connect(m_pLocationSelector, SIGNAL(clicked()), this, SLOT(sltSelectLocationButtonClicked()));
    connect(m_pSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(sltSizeSliderValueChanged(int)));
    connect(m_pSizeEditor, SIGNAL(textChanged(const QString &)), this, SLOT(sltSizeEditorTextChanged(const QString &)));
}

void UINewHDWizardPageOptions::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UINewHDWizardPageOptions::retranslateUi(this);

    /* Set 'options' page title: */
    setTitle(tr("Virtual disk file location and size"));
}

void UINewHDWizardPageOptions::initializePage()
{
    /* Retranslate page: */
    retranslateUi();

    /* Initialize name: */
    m_pLocationEditor->setText(m_strMediumName);
    /* Initialize size: */
    m_pSizeSlider->setValue(sizeMBToSlider(m_uMediumSize, m_iSliderScale));
    /* 'Size' editor should have focus initially: */
    m_pSizeEditor->setFocus();
    /* Get default extension: */
    m_strDefaultExtension = defaultExtension(field("mediumFormat").value<CMediumFormat>());
    m_strMediumPath = absoluteFilePath(toFileName(m_strMediumName, m_strDefaultExtension), m_strDefaultPath);
}

void UINewHDWizardPageOptions::cleanupPage()
{
    /* Reset widgets: */
    m_pLocationEditor->clear();
    m_pSizeSlider->setValue(0);
    /* Call for base-class: */
    UINewHDWizardPage::cleanupPage();
}

bool UINewHDWizardPageOptions::isComplete() const
{
    /* Check what current size feats the bounds & current name is not empty! */
    return m_uMediumSize >= m_uMediumSizeMin && m_uMediumSize <= m_uMediumSizeMax &&
           !m_strMediumName.trimmed().isEmpty();
}

bool UINewHDWizardPageOptions::validatePage()
{
    if (QFileInfo(m_strMediumPath).exists())
    {
        vboxProblem().sayCannotOverwriteHardDiskStorage(this, m_strMediumPath);
        return false;
    }
    return true;
}

void UINewHDWizardPageOptions::sltLocationEditorTextChanged(const QString &strText)
{
    /* Set current medium name: */
    m_strMediumName = strText;
    /* Set current medium path: */
    m_strMediumPath = absoluteFilePath(toFileName(m_strMediumName, m_strDefaultExtension), m_strDefaultPath);

    /* Notify wizard sub-system about complete status changed: */
    emit completeChanged();
}

void UINewHDWizardPageOptions::sltSelectLocationButtonClicked()
{
    /* Get current folder and filename: */
    QFileInfo fullFilePath(m_strMediumPath);
    QDir folder = fullFilePath.path();
    QString strFileName = fullFilePath.fileName();

    /* Set the first parent folder that exists as the current: */
    while (!folder.exists() && !folder.isRoot())
    {
        QFileInfo folderInfo(folder.absolutePath());
        if (folder == QDir(folderInfo.absolutePath()))
            break;
        folder = folderInfo.absolutePath();
    }

    /* But if it doesn't exists at all: */
    if (!folder.exists() || folder.isRoot())
    {
        /* Use recommended one folder: */
        QFileInfo defaultFilePath(absoluteFilePath(strFileName, m_strDefaultPath));
        folder = defaultFilePath.path();
    }

    /* Prepare backends list: */
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    QStringList validExtensionList;
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == KDeviceType_HardDisk)
            validExtensionList << QString("*.%1").arg(fileExtensions[i]);
    /* Compose full filter list: */
    QString strBackendsList = QString("%1 (%2)").arg(mediumFormat.GetName()).arg(validExtensionList.join(" "));

    /* Open corresponding file-dialog: */
    QString strChosenFilePath = QIFileDialog::getSaveFileName(folder.absoluteFilePath(strFileName),
                                                              strBackendsList, this,
                                                              tr("Select a file for the new hard disk image file"));

    /* If there was something really chosen: */
    if (!strChosenFilePath.isEmpty())
    {
        /* If valid file extension is missed, append it: */
        if (QFileInfo(strChosenFilePath).completeSuffix().isEmpty())
            strChosenFilePath += QString(".%1").arg(m_strDefaultExtension);
        m_pLocationEditor->setText(QDir::toNativeSeparators(strChosenFilePath));
        m_pLocationEditor->selectAll();
        m_pLocationEditor->setFocus();
    }
}

void UINewHDWizardPageOptions::sltSizeSliderValueChanged(int iValue)
{
    /* Update currently stored size: */
    m_uMediumSize = sliderToSizeMB(iValue, m_iSliderScale);
    /* Update tooltip: */
    updateSizeToolTip(m_uMediumSize);
    /* Notify size-editor about size had changed preventing callback: */
    m_pSizeEditor->blockSignals(true);
    m_pSizeEditor->setText(vboxGlobal().formatSize(m_uMediumSize));
    m_pSizeEditor->blockSignals(false);

    /* Notify wizard sub-system about complete status changed: */
    emit completeChanged();
}

void UINewHDWizardPageOptions::sltSizeEditorTextChanged(const QString &strValue)
{
    /* Update currently stored size: */
    m_uMediumSize = vboxGlobal().parseSize(strValue);
    /* Update tooltip: */
    updateSizeToolTip(m_uMediumSize);
    /* Notify size-slider about size had changed preventing callback: */
    m_pSizeSlider->blockSignals(true);
    m_pSizeSlider->setValue(sizeMBToSlider(m_uMediumSize, m_iSliderScale));
    m_pSizeSlider->blockSignals(false);

    /* Notify wizard sub-system about complete status changed: */
    emit completeChanged();
}

/* static */
QString UINewHDWizardPageOptions::absoluteFilePath(const QString &strFileName, const QString &strDefaultPath)
{
    /* Wrap file-info around received file name: */
    QFileInfo fileInfo(strFileName);
    /* If path-info is relative or there is no path-info at all: */
    if (fileInfo.fileName() == strFileName || fileInfo.isRelative())
    {
        /* Resolve path on the basis of default path we have: */
        fileInfo = QFileInfo(strDefaultPath, strFileName);
    }
    /* Return full absolute hard disk file path: */
    return QDir::toNativeSeparators(fileInfo.absoluteFilePath());
}

/* static */
QString UINewHDWizardPageOptions::toFileName(const QString &strName, const QString &strExtension)
{
    /* Convert passed name to native separators (it can be full, actually): */
    QString strFileName = QDir::toNativeSeparators(strName);

    /* Remove all trailing dots to avoid multiple dots before extension: */
    int iLen;
    while (iLen = strFileName.length(), iLen > 0 && strFileName[iLen - 1] == '.')
        strFileName.truncate(iLen - 1);

    /* Add passed extension if its not done yet: */
    if (QFileInfo(strFileName).completeSuffix().toLower() != strExtension)
        strFileName += QString(".%1").arg(strExtension);

    /* Return result: */
    return strFileName;
}

/* static */
QString UINewHDWizardPageOptions::defaultExtension(CMediumFormat mediumFormat)
{
    /* Load extension / device list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == KDeviceType_HardDisk)
            return fileExtensions[i].toLower();
    AssertMsgFailed(("Extension can't be NULL!\n"));
    return QString();
}

/* static */
int UINewHDWizardPageOptions::log2i(qulonglong uValue)
{
    int iPower = -1;
    while (uValue)
    {
        ++iPower;
        uValue >>= 1;
    }
    return iPower;
}

/* static */
int UINewHDWizardPageOptions::sizeMBToSlider(qulonglong uValue, int iSliderScale)
{
    int iPower = log2i(uValue);
    qulonglong uTickMB = qulonglong (1) << iPower;
    qulonglong uTickMBNext = qulonglong (1) << (iPower + 1);
    int iStep = (uValue - uTickMB) * iSliderScale / (uTickMBNext - uTickMB);
    return iPower * iSliderScale + iStep;
}

/* static */
qulonglong UINewHDWizardPageOptions::sliderToSizeMB(int uValue, int iSliderScale)
{
    int iPower = uValue / iSliderScale;
    int iStep = uValue % iSliderScale;
    qulonglong uTickMB = qulonglong (1) << iPower;
    qulonglong uTickMBNext = qulonglong (1) << (iPower + 1);
    return uTickMB + (uTickMBNext - uTickMB) * iStep / iSliderScale;
}

void UINewHDWizardPageOptions::updateSizeToolTip(qulonglong uSize)
{
    QString strToolTip = tr("<nobr>%1 (%2 B)</nobr>").arg(vboxGlobal().formatSize(uSize)).arg(uSize);
    m_pSizeSlider->setToolTip(strToolTip);
    m_pSizeEditor->setToolTip(strToolTip);
}

UINewHDWizardPageSummary::UINewHDWizardPageSummary()
{
    /* Decorate page: */
    Ui::UINewHDWizardPageSummary::setupUi(this);

    /* Register CMedium class: */
    qRegisterMetaType<CMedium>();

    /* Register 'hardDisk' field: */
    registerField("hardDisk", this, "hardDisk");

    /* Disable the background painting of the summary widget: */
    m_pSummaryText->viewport()->setAutoFillBackground(false);
}

void UINewHDWizardPageSummary::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UINewHDWizardPageSummary::retranslateUi(this);

    /* Set 'summary' page title: */
    setTitle(tr("Summary"));

    /* Compose common summary: */
    QString strSummary;

    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    qulonglong uVariant = field("mediumVariant").toULongLong();
    QString strMediumPath = field("mediumPath").toString();
    QString sizeFormatted = VBoxGlobal::formatSize(field("mediumSize").toULongLong());
    QString sizeUnformatted = tr("%1 B").arg(field("mediumSize").toULongLong());

    strSummary += QString
    (
        "<tr><td><nobr>%1: </nobr></td><td><nobr>%2</nobr></td></tr>"
        "<tr><td><nobr>%3: </nobr></td><td><nobr>%4</nobr></td></tr>"
        "<tr><td><nobr>%5: </nobr></td><td><nobr>%6</nobr></td></tr>"
        "<tr><td><nobr>%7: </nobr></td><td><nobr>%8 (%9)</nobr></td></tr>"
    )
    .arg(tr("File type", "summary"), mediumFormat.isNull() ? QString() : VBoxGlobal::removeAccelMark(UINewHDWizardPageFormat::fullFormatName(mediumFormat.GetName())))
    .arg(tr("Details", "summary"), UINewHDWizardPageVariant::variantToString(uVariant))
    .arg(tr("Location", "summary"), strMediumPath)
    .arg(tr("Size", "summary"), sizeFormatted, sizeUnformatted);

    /* Feat summary to 4 lines: */
    setSummaryFieldLinesNumber(m_pSummaryText, 4);

    m_pSummaryText->setText("<table cellspacing=0 cellpadding=0>" + strSummary + "</table>");

    m_pLabel2->setText(tr("If the above settings are correct, press the <b>%1</b> button. "
                          "Once you press it a new virtual disk file will be created.")
                          .arg(VBoxGlobal::replaceHtmlEntities(VBoxGlobal::removeAccelMark(wizard()->buttonText(QWizard::FinishButton)))));
}

void UINewHDWizardPageSummary::initializePage()
{
    /* Retranslate page: */
    retranslateUi();

    /* Summary should have focus initially: */
    m_pSummaryText->setFocus();
}

bool UINewHDWizardPageSummary::validatePage()
{
    /* Start performing long-time operation: */
    startProcessing();
    /* Try to construct hard disk: */
    bool fResult = createHardDisk();
    /* Finish performing long-time operation: */
    endProcessing();
    /* Return operation result: */
    return fResult;
}

bool UINewHDWizardPageSummary::createHardDisk()
{
    /* Gather attributes: */
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    qulonglong uVariant = field("mediumVariant").toULongLong();
    QString strMediumPath = field("mediumPath").toString();
    qulonglong uSize = field("mediumSize").toULongLong();

    AssertReturn(!strMediumPath.isNull(), false);
    AssertReturn(uSize > 0, false);

    CVirtualBox vbox = vboxGlobal().virtualBox();

    CProgress progress;

    CMedium hardDisk = vbox.CreateHardDisk(mediumFormat.GetName(), strMediumPath);

    if (!vbox.isOk())
    {
        vboxProblem().cannotCreateHardDiskStorage(this, vbox, strMediumPath, hardDisk, progress);
        return false;
    }

    progress = hardDisk.CreateBaseStorage(uSize, uVariant);

    if (!hardDisk.isOk())
    {
        vboxProblem().cannotCreateHardDiskStorage(this, vbox, strMediumPath, hardDisk, progress);
        return false;
    }

    vboxProblem().showModalProgressDialog(progress, windowTitle(), ":/progress_media_create_90px.png", this, true);

    if (progress.GetCanceled())
        return false;

    if (!progress.isOk() || progress.GetResultCode() != 0)
    {
        vboxProblem().cannotCreateHardDiskStorage(this, vbox, strMediumPath, hardDisk, progress);
        return false;
    }

    /* Inform everybody there is a new medium: */
    vboxGlobal().addMedium(VBoxMedium(CMedium(hardDisk), VBoxDefs::MediumType_HardDisk, KMediumState_Created));

    m_HardDisk = hardDisk;
    return true;
}

#include "UINewHDWizard.moc"