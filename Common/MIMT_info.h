/*!
 **********************************************************************************************************************
 *
 * \file   MIMT_info.h
 *
 * \brief  A brief description of what this header file is.
 *
 * \details Every file that is part of a documented module has to have a file block, else it will not show
 *          up in the Doxygen "Modules" section.
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 *
 * Copyright (c) 2017 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *
**********************************************************************************************************************/

#ifndef MIMT_INFO_H
#define MIMT_INFO_H

/* INCLUDE FILES */
#include "project.h"
#include "HEEP_util.h"

/* MACRO DEFINITIONS */


/* TYPE DEFENITIONS */


/* CONSTANTS */


/* VARIABLE DEFENITIONS */


/* FUNCTION PROTOTYPES */
extern returnStatus_t MIMTINFO_init(void);
extern uint16_t       MIMTINFO_getFctModuleTestDate(void);
extern void           MIMTINFO_setFctModuleTestDate(uint16_t uFctModuleTestDate);
extern uint16_t       MIMTINFO_getFctEnclosureTestDate(void);
extern void           MIMTINFO_setFctEnclosureTestDate(uint16_t uFctEnclosureTestDate);
extern uint16_t       MIMTINFO_getIntegrationSetupDate(void);
extern void           MIMTINFO_setIntegrationSetupDate(uint16_t uIntegrationSetupDate);
extern uint32_t       MIMTINFO_getFctModuleProgramVersion(void);
extern void           MIMTINFO_setFctModuleProgramVersion(uint32_t uFctModuleProgramVersion);
extern uint32_t       MIMTINFO_getFctEnclosureProgramVersion(void);
extern void           MIMTINFO_setFctEnclosureProgramVersion(uint32_t uFctEnclosureProgramVersion);
extern uint32_t       MIMTINFO_getIntegrationProgramVersion(void);
extern void           MIMTINFO_setIntegrationProgramVersion(uint32_t uIntegrationProgramVersion);
extern uint32_t       MIMTINFO_getFctModuleDatabaseVersion(void);
extern void           MIMTINFO_setFctModuleDatabaseVersion(uint32_t uFctModuleDatabaseVersion);
extern uint32_t       MIMTINFO_getFctEnclosureDatabaseVersion(void);
extern void           MIMTINFO_setFctEnclosureDatabaseVersion(uint32_t uFctEnclosureDatabaseVersion);
extern uint32_t       MIMTINFO_getIntegrationDatabaseVersion(void);
extern void           MIMTINFO_setIntegrationDatabaseVersion(uint32_t uIntegrationDatabaseVersion);
extern uint64_t       MIMTINFO_getDataConfigurationDocumentVersion(void);
extern void           MIMTINFO_setDataConfigurationDocumentVersion(uint64_t uEepromMapDocumentVersion);
extern uint64_t       MIMTINFO_getManufacturerNumber(void);
extern void           MIMTINFO_setManufacturerNumber(uint64_t uManufacturerNumber);
extern uint32_t       MIMTINFO_getRepairInformation(void);
extern void           MIMTINFO_setRepairInformation(uint32_t uRepairInformation);
extern returnStatus_t MIMTINFO_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);

#endif /* MIMT_INFO_H */
