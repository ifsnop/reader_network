reader_network 0.79 - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

[![Build Status](https://travis-ci.org/ifsnop/reader_network.svg?branch=master)](https://travis-ci.org/ifsnop/reader_network)
[![Coverity Analysis](https://scan.coverity.com/projects/2418/badge.svg)](https://scan.coverity.com/projects/2418?tab=overview)

Work in progress. Although I use this software everyday (in a realtime quality
monitorization environment), this software is EXPERIMENTAL.

Copyright (C) 2002-2022 Diego Torres <diego dot torres at gmail dot com>

See bin/conf/example.conf as a working example of the reader_network
config file. All options are detailed inside.

If you want to understand what ASTERIX is and how it is used, you
should check the following documents from EUROCONTROL and the
ASTERIX EUROCONTORL website

http://www.eurocontrol.int/asterix/public/subsite_homepage/homepage.html
http://www.eurocontrol.int/asterix/public/standard_page/documents.html

EUROCONTROL STANDARD DOCUMENT FOR RADAR DATA EXCHANGE Part 2a Transmission of Monoradar Data Target Reports
SUR.ET1.ST05.2000-STD-02a-01 1.1 August 2002
http://www.eurocontrol.int/asterix/gallery/content/public/documents/astx2a12.pdf

EUROCONTROL STANDARD DOCUMENT FOR SURVEILLANCE DATA EXCHANGE Part 2b Transmission of Monoradar Service Messages
SUR.ET1.ST05.2000-STD-02b-01 1.26 November 2000
http://www.eurocontrol.int/asterix/gallery/content/public/documents/astx2b1.pdf

What is ASTERIX ?

ASTERIX is the EUROCONTROL Standard for the exchange of Surveillance related data.
The acronym stands for "All Purpose STructured Eurocontrol SuRveillance Information EXchange".

ASTERIX provides a structured approach to a message format to be applied in the exchange of surveillance related information for various applications. Developed by the SuRveillance Data Exchange Task Force (RDE-TF) with its multinational participation, it ensures a common data representation, thereby facilitating the exchange of surveillance data in an international context.

Having started as a EUROCONTROL development (see the "history" section for further details), ASTERIX is now applied worldwide.

ASTERIX defines a structured approach to the encoding of surveillance data. Categories group the information related to a specific application. They consist of a number of data items which are defined in the ASTERIX documents down to the bit-level.

DOCUMENTS

EUROCONTROL STANDARD DOCUMENT FOR RADAR SURVEILLANCE IN EN-ROUTE AIRSPACE AND MAJOR TERMINAL AREAS
SUR.ET1.ST01.1000-STD-01-01 1.0 March 1997

EUROCONTORL RADAR SENSOR PERFORMANCE ANALYSIS
SUR.ET1.ST03.1000-STD-01-01 0.1 June 1997

DRAFT IMPLEMENTING RULE ON SURVEILLANCE PERFORMANCE AND INTEROPERABILITY REQUIREMENTS EUROCONTROL. European mode S Station Surveillance Coordination Interface Control Document. SUR/MODES/EMS/ICD-01.

European mode S Station Functional Specification. SUR/MODES/EMS/SPE-01. Version 3.11

Specification for ATM Surveillance System Performance EUROCONTROL-SPEC-0147 0.35 01/09/2011
