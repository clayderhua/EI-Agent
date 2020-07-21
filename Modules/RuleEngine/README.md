# RMM RuleEngine Plugin
Full RuleEngine Package, include EIService SDK and WAPI libraries.

Decuments:
	 
## PATENTS
----

## COMPILATION
----
 * Windows - Visual Studio 2008 
   1. Open "SRP-Plugin.sln"
   2. Rebuild all solution by click build->Rebuild Solution
   3. Copy the binary in Relese Folder into RMM Agent installed directory
   4. Restart the RMM Agent to load the plugins.
   
 * Linux - CentOS
   1. Execute pre-install-centos.sh as a super user 
   2. Execute build-srpplugin.sh
   3. Copy the binary in Relese Folder into RMM Agent installed directory
   4. Restart the RMM Agent to load the plugins.
   
 * Linux - Ubuntu
   1. Execute pre-install-ubuntu.sh as a super user 
   2. Execute build-srpplugin.sh
   3.  Copy the binary in Relese Folder into RMM Agent installed directory
   4. Restart the RMM Agent to load the plugins.
  
 * To compile this package under other Unix systems, user need install or pre-compile the following libraries:
   - openssl-1.0.1h
   - curl
   - cmake
   - autoconf
   - automake
   - make
   - libx86 
   
## Hardware requirements
----

* CPU architecture
  - x86
  - ARM
 
## OS
----

 * Windows
   - XP, 7, 8, 10

 * Linux
   - Ubuntu ( 14.04.2 X64)
   - CentOS (6.5 X86)
   - Yocto
 
## PROBLEMS
----

## SUPPORT
----

 * [Advantech IoT Developer Forum](http://iotforum.advantech.com/)
 * [WISE-PaaS Documents](http://wise-paas-documentation.docs.wise-paas.com/Documentation/#!index.md)
 
## License
----

Copyright (c) Advantech Corporation. All rights reserved.

