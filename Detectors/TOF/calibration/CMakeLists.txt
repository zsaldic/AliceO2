# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

o2_add_library(TOFCalibration
               SOURCES src/CalibTOFapi.cxx src/CalibTOF.cxx src/CollectCalibInfoTOF.cxx
               PUBLIC_LINK_LIBRARIES O2::DataFormatsTOF O2::TOFBase
                                     O2::CCDB)

o2_target_root_dictionary(TOFCalibration
                          HEADERS include/TOFCalibration/CalibTOFapi.h include/TOFCalibration/CalibTOF.h
                                  include/TOFCalibration/CollectCalibInfoTOF.h)
