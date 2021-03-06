//=============================================================================================================
/**
* @file     realtimeevokedsetwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Definition of the RealTimeEvokedSetWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedsetwidget.h"

#include <disp/viewers/quickcontrolview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/filterview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/channeldatasettingsview.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/averagingsettingsview.h>

#include <scMeas/realtimeevokedset.h>

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QLabel>
#include <QToolBox>
#include <QDate>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGraphicsItem>
#include <QDir>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSetWidget::RealTimeEvokedSetWidget(QSharedPointer<RealTimeEvokedSet> pRTESet,
                                                 QSharedPointer<QTime> &pTime,
                                                 QWidget* parent)
: MeasurementWidget(parent)
, m_bInitialized(false)
, m_pRTESet(pRTESet)
{
    Q_UNUSED(pTime)
    //qRegisterMetaType<SCDISPLIB::AverageInfoMap>("SCDISPLIB::AverageInfoMap");
    qRegisterMetaTypeStreamOperators<SCDISPLIB::AverageInfoMap>("SCDISPLIB::AverageInfoMap");

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the region selection widget (F11)"),this);
    m_pActionSelectSensors->setShortcut(tr("F11"));
    m_pActionSelectSensors->setStatusTip(tr("Show the region selection widget (F11)"));
    connect(m_pActionSelectSensors.data(), &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(false);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showQuickControlWidget);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(false);

    //Create GUI
    m_pRTESetLayout = new QVBoxLayout(this);

    //Set acquire label
    m_pLabelInit= new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setBold(true);
    font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRTESetLayout->addWidget(m_pLabelInit);

    //Create toolboxes with butterfly and 2D layout plot
    m_pToolBox = new QToolBox(this);
    m_pToolBox->hide();

    //Butterfly
    m_pButterflyView = new ButterflyView(this);
    m_pButterflyView->installEventFilter(this);

    //2D layout plot
    m_pAverageLayoutView = new AverageLayoutView(this);
    //m_pAverageLayoutView->installEventFilter(this);

    m_pToolBox->insertItem(0, m_pButterflyView, QIcon(), "Butterfly plot");
    m_pToolBox->insertItem(0, m_pAverageLayoutView, QIcon(), "2D Layout plot");

    m_pRTESetLayout->addWidget(m_pToolBox);

    // Init quick control view
    m_pQuickControlView = QSharedPointer<QuickControlView>::create("RT Averaging", Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint, this);
    QSettings settings;
    m_pQuickControlView->setOpacityValue(settings.value(QString("RTESW/%1/viewOpacity").arg(m_pRTESet->getName()), 100).toInt());
    m_pActionQuickControl->setVisible(true);

    // Quick control average selection
    QList<QSharedPointer<QWidget> > lControlWidgets = m_pRTESet->getControlWidgets();
    if(!lControlWidgets.isEmpty()) {
        if(lControlWidgets.first()) {
            m_pAveragingSettingsView = qSharedPointerDynamicCast<DISPLIB::AveragingSettingsView>(lControlWidgets.first());
            m_pQuickControlView->addGroupBoxWithTabs(lControlWidgets.first(), "Averaging", "Settings");
        }
    }

    //set layouts
    this->setLayout(m_pRTESetLayout);
}


//*************************************************************************************************************

RealTimeEvokedSetWidget::~RealTimeEvokedSetWidget()
{
    // Store Settings
    if(!m_pRTESet->getName().isEmpty())
    {
        QString t_sRTESName = m_pRTESet->getName();

        QSettings settings;

        //Store modalities
        if(m_pButterflyView) {
            QList<Modality> qListModalities = m_pButterflyView->getModalities();
            for(qint32 i = 0; i < qListModalities.size(); ++i) {
                settings.setValue(QString("RTESW/%1/%2/active").arg(t_sRTESName).arg(qListModalities[i].m_sName), qListModalities[i].m_bActive);
                settings.setValue(QString("RTESW/%1/%2/norm").arg(t_sRTESName).arg(qListModalities[i].m_sName), qListModalities[i].m_fNorm);
            }
        }

        //Store filter
        if(m_pFilterView) {
            FilterData filter = m_pFilterView->getUserDesignedFilter();

            settings.setValue(QString("RTESW/%1/filterHP").arg(t_sRTESName), filter.m_dHighpassFreq);
            settings.setValue(QString("RTESW/%1/filterLP").arg(t_sRTESName), filter.m_dLowpassFreq);
            settings.setValue(QString("RTESW/%1/filterOrder").arg(t_sRTESName), filter.m_iFilterOrder);
            settings.setValue(QString("RTESW/%1/filterType").arg(t_sRTESName), (int)filter.m_Type);
            settings.setValue(QString("RTESW/%1/filterDesignMethod").arg(t_sRTESName), (int)filter.m_designMethod);
            settings.setValue(QString("RTESW/%1/filterTransition").arg(t_sRTESName), filter.m_dParksWidth*(filter.m_sFreq/2));
            settings.setValue(QString("RTESW/%1/filterUserDesignActive").arg(t_sRTESName), m_pFilterView->userDesignedFiltersIsActive());
            settings.setValue(QString("RTESW/%1/filterChannelType").arg(t_sRTESName), m_pFilterView->getChannelType());
        }

        //Store scaling
        if(m_pEvokedSetModel) {
            QMap<qint32, float> qMapChScaling = m_pEvokedSetModel->getScaling();

            if(qMapChScaling.contains(FIFF_UNIT_T)) {
                settings.setValue(QString("RTESW/%1/scaleMAG").arg(t_sRTESName), qMapChScaling[FIFF_UNIT_T]);
            }

            if(qMapChScaling.contains(FIFF_UNIT_T_M)) {
                settings.setValue(QString("RTESW/%1/scaleGRAD").arg(t_sRTESName), qMapChScaling[FIFF_UNIT_T_M]);
            }

            if(qMapChScaling.contains(FIFFV_EEG_CH)) {
                settings.setValue(QString("RTESW/%1/scaleEEG").arg(t_sRTESName), qMapChScaling[FIFFV_EEG_CH]);
            }

            if(qMapChScaling.contains(FIFFV_EOG_CH)) {
                settings.setValue(QString("RTESW/%1/scaleEOG").arg(t_sRTESName), qMapChScaling[FIFFV_EOG_CH]);
            }

            if(qMapChScaling.contains(FIFFV_STIM_CH)) {
                settings.setValue(QString("RTESW/%1/scaleSTIM").arg(t_sRTESName), qMapChScaling[FIFFV_STIM_CH]);
            }

            if(qMapChScaling.contains(FIFFV_MISC_CH)) {
                settings.setValue(QString("RTESW/%1/scaleMISC").arg(t_sRTESName), qMapChScaling[FIFFV_MISC_CH]);
            }
        }

        //Store selected layout file
        if(m_pChannelSelectionView) {
            settings.setValue(QString("RTESW/%1/selectedLayoutFile").arg(t_sRTESName), m_pChannelSelectionView->getCurrentLayoutFile());
        }

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("RTESW/%1/selectedView").arg(t_sRTESName), m_pToolBox->currentIndex());
        }

        //Store average colors per type
        if(m_pAverageLayoutView) {
            QVariant data;
            SCDISPLIB::AverageInfoMap avrMap = m_pAverageLayoutView->getAverageInformationMap();
            data.setValue(avrMap);
            settings.setValue(QString("RTESW/%1/averageInfoMap").arg(t_sRTESName), data);
        }

        //Store signal and background colors
        if(m_pQuickControlView) {
            settings.setValue(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), m_pButterflyView->getBackgroundColor());
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::update(SCMEASLIB::Measurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::getData()
{
    if(!m_bInitialized) {
        if(m_pRTESet->isInitialized()) {
            m_qListChInfo = m_pRTESet->chInfo();
            m_pFiffInfo = m_pRTESet->info();

            if(!m_pRTESet->getValue()->evoked.isEmpty()) {
                m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();
            }

            init();

            m_pEvokedSetModel->updateData();
        }
    }

    if(m_bInitialized) {
        //Check if block size has changed, if yes update the filter
        if(!m_pRTESet->getValue()->evoked.isEmpty()) {
            if(m_iMaxFilterTapSize != m_pRTESet->getValue()->evoked.first().data.cols()) {
                m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();

                m_pFilterView->setWindowSize(m_iMaxFilterTapSize);
                m_pFilterView->setMaxFilterTaps(m_iMaxFilterTapSize);
            }
        }

        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *(m_pFiffInfo.data());
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);

        m_pEvokedSetModel->updateData();

        // Update number of trials in GUI
        if(m_pAveragingSettingsView) {
            m_pAveragingSettingsView->setDetectedEpochs(pEvokedSet);
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::init()
{
    if(m_pFiffInfo) {
        QSettings settings;
        QString t_sRTESName = m_pRTESet->getName();

        //Remove temporary label and show actual average display
        m_pRTESetLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();
        m_pToolBox->show();
        m_pActionSelectSensors->setVisible(true);

        //Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("RTESW/%1/selectedView").arg(t_sRTESName), 0).toInt());

        //Init data model
        m_pEvokedSetModel = EvokedSetModel::SPtr::create(this);
        m_pEvokedSetModel->setChannelColors(m_pRTESet->chColor());

        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *m_pFiffInfo.data();
        m_pEvokedSetModel->setEvokedSet(pEvokedSet, true);

        //Init modalities and scaling
        bool sel, hasMag = false, hasGrad = false, hasEEG = false, hasEog = false, hasStim = false, hasMisc = false;
        float val = 1e-11f;
        QMap<qint32, float> qMapChScaling;
        QList<Modality> qListModalities;

        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i) {
            if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH) {
                if(!hasMag && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T) {
                    //Modality
                    sel = settings.value(QString("RTESW/%1/MAG/active").arg(t_sRTESName), true).toBool();
                    val = settings.value(QString("RTESW/%1/MAG/norm").arg(t_sRTESName), 1e-11f).toFloat();
                    qListModalities.append(Modality("MAG",sel,val));

                    //Scaling
                    val = settings.value(QString("RTESW/%1/scaleMAG").arg(t_sRTESName), 1e-11f).toFloat();
                    qMapChScaling.insert(FIFF_UNIT_T, val);

                    hasMag = true;
                } else if(!hasGrad && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M) {
                    //Modality
                    sel = settings.value(QString("RTESW/%1/GRAD/active").arg(t_sRTESName), true).toBool();
                    val = settings.value(QString("RTESW/%1/GRAD/norm").arg(t_sRTESName), 1e-10f).toFloat();
                    qListModalities.append(Modality("GRAD",sel,val));

                    //Scaling
                    val = settings.value(QString("RTESW/%1/scaleGRAD").arg(t_sRTESName), 1e-10f).toFloat();
                    qMapChScaling.insert(FIFF_UNIT_T_M, val);

                    hasGrad = true;
                }
            } else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH) {
                //Modality
                sel = settings.value(QString("RTESW/%1/EEG/active").arg(t_sRTESName), true).toBool();
                val = settings.value(QString("RTESW/%1/EEG/norm").arg(t_sRTESName), 1e-4f).toFloat();
                qListModalities.append(Modality("EEG",sel,val));

                //Scaling
                val = settings.value(QString("RTESW/%1/scaleEEG").arg(t_sRTESName), 1e-4f).toFloat();
                qMapChScaling.insert(FIFFV_EEG_CH, val);

                hasEEG = true;
            } else if(!hasEog && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH) {
                //Modality
                sel = settings.value(QString("RTESW/%1/EOG/active").arg(t_sRTESName), true).toBool();
                val = settings.value(QString("RTESW/%1/EOG/norm").arg(t_sRTESName), 1e-3f).toFloat();
                qListModalities.append(Modality("EOG",sel,val));

                //Scaling
                val = settings.value(QString("RTESW/%1/scaleEOG").arg(t_sRTESName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_EOG_CH, val);

                hasEog = true;
            } else if(!hasStim && m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                //Scaling only we do not need it as a modality
                val = settings.value(QString("RTESW/%1/scaleSTIM").arg(t_sRTESName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_STIM_CH, val);

                hasStim = true;
            } else if(!hasMisc && m_pFiffInfo->chs[i].kind == FIFFV_MISC_CH) {
                //Modality
                sel = settings.value(QString("RTESW/%1/MISC/active").arg(t_sRTESName), true).toBool();
                val = settings.value(QString("RTESW/%1/MISC/norm").arg(t_sRTESName), 1e-3f).toFloat();
                qListModalities.append(Modality("MISC",sel,val));

                //Scaling
                val = settings.value(QString("RTESW/%1/scaleMISC").arg(t_sRTESName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_MISC_CH, val);

                hasMisc = true;
            }
        }

        m_pEvokedSetModel->setScaling(qMapChScaling);

        //Init filter window
        m_pFilterView = FilterView::SPtr::create(this, Qt::Window);

        connect(m_pFilterView.data(), static_cast<void (FilterView::*)(QString)>(&FilterView::applyFilter),
                m_pEvokedSetModel.data(),static_cast<void (EvokedSetModel::*)(QString)>(&EvokedSetModel::setFilterChannelType));

        connect(m_pFilterView.data(), &FilterView::filterChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::filterChanged);

        m_pFilterView->init(m_pFiffInfo->sfreq);
        m_pFilterView->setWindowSize(m_iMaxFilterTapSize);
        m_pFilterView->setMaxFilterTaps(m_iMaxFilterTapSize);
        m_pFilterView->setSamplingRate(m_pFiffInfo->sfreq);

        //Set stored filter settings from last session
        m_pFilterView->setFilterParameters(settings.value(QString("RTESW/%1/filterHP").arg(t_sRTESName), 5.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterLP").arg(t_sRTESName), 40.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterOrder").arg(t_sRTESName), 128).toInt(),
                                                settings.value(QString("RTESW/%1/filterType").arg(t_sRTESName), 2).toInt(),
                                                settings.value(QString("RTESW/%1/filterDesignMethod").arg(t_sRTESName), 0).toInt(),
                                                settings.value(QString("RTESW/%1/filterTransition").arg(t_sRTESName), 5.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterUserDesignActive").arg(t_sRTESName), false).toBool(),
                                                settings.value(QString("RTESW/%1/filterChannelType").arg(t_sRTESName), "MEG").toString());

        //Init channel selection manager
        m_pChannelInfoModel = QSharedPointer<ChannelInfoModel>(new ChannelInfoModel(m_pFiffInfo, this));
        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create(this, m_pChannelInfoModel, Qt::Window);

        //Connect channel info model
        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pButterflyView.data(), &ButterflyView::showSelectedChannelsOnly);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::selectionChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::channelSelectionManagerChanged);

        m_pChannelInfoModel->fiffInfoChanged(m_pFiffInfo);
        m_pChannelSelectionView->setCurrentLayoutFile(settings.value(QString("RTESW/%1/selectedLayoutFile").arg(t_sRTESName), "babymeg-mag-inner-layer.lout").toString());

        // Quick control scaling
        ScalingView* pScalingView = new ScalingView();
        pScalingView->init(m_pEvokedSetModel->getScaling());
        m_pQuickControlView->addGroupBox(pScalingView, "Scaling");

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setScaling);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setScaleMap);

        // Quick control projectors
        ProjectorsView* pProjectorsView = new ProjectorsView();
        pProjectorsView->init(m_pFiffInfo);
        m_pQuickControlView->addGroupBoxWithTabs(pProjectorsView, "Noise", "SSP");

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateProjection);

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        //Activate projectors by default
        m_pEvokedSetModel->updateProjection();

        // Quick control compensators
        CompensatorView* pCompensatorView = new CompensatorView();
        pCompensatorView->init(m_pFiffInfo);
        m_pQuickControlView->addGroupBoxWithTabs(pCompensatorView, "Noise", "Comp");

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateCompensator);

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        // Quick control filter settings
        FilterSettingsView* pFilterSettingsView = new FilterSettingsView();
        m_pQuickControlView->addGroupBoxWithTabs(pFilterSettingsView, "Noise", "Filter");

        connect(m_pFilterView.data(), &FilterView::activationCheckBoxListChanged,
                pFilterSettingsView, &FilterSettingsView::filterGroupChanged);

        connect(pFilterSettingsView, &FilterSettingsView::showFilterOptions,
                this, &RealTimeEvokedSetWidget::showFilterWidget);

        pFilterSettingsView->filterGroupChanged(m_pFilterView->getActivationCheckBoxList());

        // Quick control channel data settings
        ChannelDataSettingsView* pChannelDataSettingsView = new ChannelDataSettingsView();
        pChannelDataSettingsView->init(QStringList() << "screenshot" << "backgroundColor");
        m_pQuickControlView->addGroupBoxWithTabs(pChannelDataSettingsView, "Other", "View");

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::backgroundColorChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setBackgroundColor);

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::backgroundColorChanged,
                m_pButterflyView.data(), &ButterflyView::setBackgroundColor);

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::makeScreenshot,
                this, &RealTimeEvokedSetWidget::onMakeScreenshot);

        QColor backgroundDefault = Qt::black;
        pChannelDataSettingsView->setSignalBackgroundColors(QColor(),
                                                            settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());

        // Quick control channel data settings
        ModalitySelectionView* pModalitySelectionView = new ModalitySelectionView();
        pModalitySelectionView->init(m_pFiffInfo);
        pModalitySelectionView->setModalities(qListModalities);
        m_pQuickControlView->addGroupBoxWithTabs(pModalitySelectionView, "Other", "Modalities");

        connect(pModalitySelectionView, &ModalitySelectionView::modalitiesChanged,
                m_pButterflyView.data(), &ButterflyView::setModalities);



        AverageSelectionView* pAverageSelectionView = new AverageSelectionView();
        pAverageSelectionView->init();
        m_pQuickControlView->addGroupBoxWithTabs(pAverageSelectionView, "Averaging", "Selection");

        //Handle averages
        connect(this->m_pEvokedSetModel.data(), &EvokedSetModel::newAverageTypeReceived,
                pAverageSelectionView, &AverageSelectionView::setAverageInformationMap);

        connect(pAverageSelectionView, &AverageSelectionView::averageInformationChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setAverageInformationMap);

        connect(pAverageSelectionView, &AverageSelectionView::averageInformationChanged,
                m_pButterflyView.data(), &ButterflyView::setAverageInformationMap);

        QVariant data;
        QMap<double, QPair<QColor, QPair<QString,bool> > > emptyMap;
        data.setValue(emptyMap);
        SCDISPLIB::AverageInfoMap map = settings.value(QString("RTESW/%1/averageInfoMap").arg(t_sRTESName), data).value<SCDISPLIB::AverageInfoMap>();
        pAverageSelectionView->setAverageInformationMapOld(map);

        m_pButterflyView->setModel(m_pEvokedSetModel);
        m_pButterflyView->setChannelInfoModel(m_pChannelInfoModel);
        m_pButterflyView->setModalities(qListModalities);

        m_pAverageLayoutView->setFiffInfo(m_pFiffInfo);
        m_pAverageLayoutView->setChannelInfoModel(m_pChannelInfoModel);
        m_pAverageLayoutView->setEvokedSetModel(m_pEvokedSetModel);
        m_pAverageLayoutView->setScaleMap(m_pEvokedSetModel->getScaling());

        m_pAverageLayoutView->setBackgroundColor(settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());
        m_pButterflyView->setBackgroundColor(settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());

        m_pChannelSelectionView->updateDataView();
        m_pButterflyView->update();
        m_pAverageLayoutView->update();

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showSensorSelectionWidget()
{
    if(!m_pChannelSelectionView) {
        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create();
    }

    m_pChannelSelectionView->show();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showQuickControlWidget()
{
    m_pQuickControlView->show();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showFilterWidget(bool state)
{
    if(state) {
        if(m_pFilterView->isActiveWindow())
            m_pFilterView->hide();
        else {
            m_pFilterView->activateWindow();
            m_pFilterView->show();
        }
    } else {
        m_pFilterView->hide();
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    //Handle the butterfly plot and 2D layout plot differently
    QString fileName;

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "2D Layout plot") {
        if(imageType.contains("SVG")) {
            fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.svg").arg(sDate).arg(sTime);
        } else if(imageType.contains("PNG")) {
            fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.png").arg(sDate).arg(sTime);
        }
    }

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "Butterfly plot") {
        if(imageType.contains("SVG")) {
            fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.svg").arg(sDate).arg(sTime);
        } else if(imageType.contains("PNG")) {
            fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.png").arg(sDate).arg(sTime);
        }
    }

    m_pButterflyView->takeScreenshot(fileName);
}


//*************************************************************************************************************

bool RealTimeEvokedSetWidget::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pButterflyView || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pEvokedSetModel->toggleFreeze();
    }
    return false;
}

