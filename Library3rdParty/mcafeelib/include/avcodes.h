/*
 *
 * Copyright (c) 1996-2018 McAfee, LLC. All Rights Reserved.
 *
 * Parameter codes accepted by Olympus scanning engine.
 *
 *
 */

#ifndef AVCODES__H
#define AVCODES__H

#include "avmacro.h"

/* The following is a list of all parameter codes currently accepted by
 * the scanning engine.
 */
typedef WORD AV_PARAMETERCODE;
                                    /* SCANNABLE OBJECTS: */
#define AVP_MEMORY 1                /* DOS memory. */
#define AVP_BOOTSECTOR 2            /* A boot sector on a physical device or logical drive. */
#define AVP_PARTITIONSECTOR 3       /* A partition sector on a physical device. */
#define AVP_DIRECTORY 4             /* A directory. Can include wildcard specifiers/UNC paths. */
#define AVP_FILE 5                  /* A single file. */
#define AVP_VIRUSLIST 6             /* Return list of virus names stored. */
#define AVP_OBJECT 7                /* Specify the object to be scanned */

                                    /* MAJOR FUNCTIONAL PARAMETERS: */
#define AVP_CALLBACKFN 100          /* Specify callback function. */
#define AVP_USERDEFINEDDATA 101     /* User defined data extractable during CallBack. */
#define AVP_EXTRADRIVERNAME 102     /* Specify name of extra driver, overrides default. */
#define AVP_DRIVERDIRECTORY 103     /* Specify directory where API can find virus drivers. */
/* 104                              RESERVED */
/* 105                              RESERVED */
/* 106                              RESERVED */
#define AVP_APIVER 107              /* Specify the API version. Also enables the V2 interface. */
#define AVP_ENGINELOCATION 108      /* The directory containing the engine binaries */
#define AVP_TROJANDATSET 109        /* Specify the files for use in the Trojan DAT set */
#define AVP_VIRUSDATSET 110         /* Specify the files for use in the AV DAT set */
#define AVP_PUPDATSET 111           /* Specify the files for use in the PUP DAT set */
#define AVP_EXTRADATSET 112         /* Specify the files for use in the Extra DAT set */
/* 113                              RESERVED */
/* 114                              RESERVED */
/* 115                              RESERVED */
#define AVP_RUNTIMEDATSET 116       /* Specify the files for use in the runtime DAT set */
#define AVP_LIGHTWEIGHTINIT 117     /* Perform minimum initialisation to retrieve engine and DAT version */
#define AVP_NORRTEMPFILE 118        /* Do not create a temp file for runtime refactoring data. */

                                    /* INSTANCE INFOMATION PARAMETERS */
#define AVP_POINTPRODUCTINFO 150    /* Request information specific to the product from the engine */
#define AVP_EXTENSIONLIST 151       /* Specify the extension list to retrieve */
#define AVP_EXTENSIONINFO 152       /* Retrieve an extension list */
#define AVP_REGTARGETS 153          /* Get the registry target table */
/* 154                              RESERVED */
/* 155                              RESERVED */
/* 156                              RESERVED */

                                    /* SCANNABLE OBJECT ATTRIBUTES: */
#define AVP_VIRUSLISTDRIVER 200     /* Which driver to list viruses for. Attribute is AV_DRIVERFORMATTYPE. */
#define AVP_DOSDRIVENAME 201        /* A DOS drive identifier string (for most systems attribute is "A:" - "Z:"). */
#define AVP_BIOSDEVICENAME 202      /* A BIOS device identifier string (for most systems attribute is "0" - "255"). */
#define AVP_DOSPATH 203             /* A DOS path (attribute is string, e.g. "C:\DOS\*.*") */
/* 204                              RESERVED. */
#define AVP_USERFILETABLE 205       /* A pointer to a table of function pointers for accessing a file. */
#define AVP_PACKAGETABLE 206        /* package table passed in from caller */
#define AVP_SECONDARY 207           /* This scan is a secondary scan */

                                    /* ACTION PARAMETERS: */
#define AVP_REPAIR 300              /* Repair object (for files - repair if identified o/w rename or delete). */
#define AVP_RENAME 301              /* Rename object if infected (files only). */
#define AVP_DELETE 302              /* Delete object if infected. */
#define AVP_DELETEIFNOTCONTAINED 303 /* Delete object if infected and the object is the file, */
                                     /* but not if the infected object is contained in a file with other objects. */
#define AVP_MOVE 304                /* Move object if infected (attribute is string containing path to move to). */
#define AVP_QUERYSCANNABLE 305      /* Query whether the object is worth scanning. Returns AVE_SUCCESS if it would scan. */
#define AVP_RENAMEIFNOTCONTAINED 306 /* Rename object if infected and the object is the file, */
                                     /* but not if the infected object is contained in a file with other objects. */
#define AVP_NODELETETHISFILE 307    /* Disable AVP_DELETETHISFILE */

                                    /* SCANNING PARAMETERS: */
#define AVP_HEURISTICANALYSIS 400   /* Use heuristic analysis. */
#define AVP_MACROANALYSIS 401       /* Use macro heuristics analysis. */
#define AVP_NOHEURISTICS 402        /* Disable all heuristics, including elimination. */
#define AVP_DECOMPARCHIVES 403      /* Decompress archive files (eg. PKZIP, ARJ). */
#define AVP_DECOMPEXES 404          /* Decompress compressed executables (eg. PKLite, LZexe). */
#define AVP_NODECOMPMACROS 405      /* Disable macro file scanning (eg. Word 6 files). */
#define AVP_NODECOMPDEFAULTEXES 406 /* Disable default compressed executable scanning (eg. DIET files). */
/* 407                              RESERVED */
#define AVP_REPAIRONEVIRUS 408      /* Repair one virus only. */
/* 409                              RESERVED */
#define AVP_NOVOLUMELABELS 410      /* Don't scan volume labels. */
#define AVP_UUDECODE 411            /* Check files for UUEncoded content */
#define AVP_MIME 412                /* Check files for MIME content */
#define AVP_MIMESCANLINES 413       /* Number of lines to scan for MIME once mail identified */
/* 420                              RESERVED */
/* 421                              RESERVED */
#define AVP_RECURSESUBDIRS 422      /* Recursively scan subdirectories */
#define AVP_SCANALLMACROS 423       /* Scan all files for macro viruses (i.e. not just .DOC, .DOT etc) */
#define AVP_USEREXTENSIONS 424      /* Use the attached extensions when scanning. Attribute is AV_EXTENSIONLIST pointer. */
#define AVP_NODECRYPT 425           /* Don't decrypt password protected word docs */
#define AVP_DELETEALLMACROS 426     /* On finding a virus inside a document, delete all macros */
/* 500                              RESERVED */
/* 501                              RESERVED */
#define AVP_SCANALLFILES 502        /* Scan all files regardless of extension */
/* 503                              RESERVED */
/* 504                              RESERVED */
/* 505                              RESERVED */
/* 506                              RESERVED */
/* 507                              RESERVED */
/* 508                              RESERVED */
/* 509                              RESERVED */
#define AVP_WINSAFE 511             /* Windows 3.x safe memory scanning */
/* 512                              RESERVED */
/* 513                              RESERVED */
/* 514                              RESERVED */
#define AVP_HEURISTICSONLY 515      /* Heuristic scanning only.  */
#define AVP_EXCLUDELIST 516         /* Exclude paths from scan. */
#define AVP_DOHSM 517               /* Do HSM migrated files */
/* 518                              RESERVED     */
/* 519                              RESERVED     */
#define AVP_FINDDRIVEROBJECT 520	/* UserGFS implementation for FIND driver */
#define AVP_NAMEDRIVEROBJECT 521	/* UserGFS implementation for NAMES driver */
#define AVP_REPAIRDRIVEROBJECT 522	/* UserGFS implementation for REPAIR driver */
#define AVP_EXTRADRIVEROBJECT 523	/* UserGFS implementation for EXTRA driver */
#define AVP_NOJOKES	524				/* do not scan for Jokes ... */
/* 525                               RESERVED */
/* 526                               RESERVED */
/* 527                               RESERVED */
#define AVP_FORCEMBR 550            /* Stuff */
#define AVP_PLAD 551                /* Preserve last access date of files scanned. */
#define AVP_FINDALLMACROS 552       /* Find any macro. */
#define AVP_CHECKREPAIRABILITY 553	/* do we return 'we can/cannot repair this' in an AV_infection struct */
#define AVP_USEEXTRASTREAM 554      /* Dynamic extra drivers. */

#define AVP_FORCEMS  555            /* repair BR of MS-DOS */
#define AVP_FORCENEC 556            /* repair BR of NEC PC-9800 */
#define AVP_FORCEIBM 557            /* repair BR of PC-DOS */
#define AVP_DONTINFORMOAS 558       /* Don't switch off OAS. Requested by Ed White. */

#define AVP_IGNORELINKS 559         /* Ignore symbolic links. */

#define AVP_PROGRAM 560             /* Report commercial applications that could be used maliciously */
#define AVP_SERVER 561              /* Calling application is a server */
#define AVP_SCANNTFSSTREAMS 562 	/* Scan inside NTFS steams */
#define AVP_NOMIME 563 	            /* Don't scan inside MIME */
#define AVP_NOSCRIPT 564 	        /* Don't scan for scripts (inside HTML/HTA) */
#define AVP_CHECKENDLIFE 565        /* Check for end of life */
#define AVP_PROCESS 566             /* Scan Win32 memory - parameter = process ID. */
/* 567                              RESERVED */
#define AVP_SCANALLPROCESSES 568    /* Scan Win32 memory - all processes */
#define AVP_NORECALL 569            /* Win32 - files opened with FILE_FLAG_NO_RECALL flag*/
#define AVP_NOBACKUPSEMANTICS 570   /* Win32 - switch off FILE_FLAG_BACKUP_SEMANTICS */
#define AVP_OFFLINECACHEREADSIZE 571   /* Win32 - user defined offline cache size per read */
#define AVP_MAXOFFLINECACHEPERFILE 572 /* Win32 - max size per file for offline cache */
/* 573                              RESERVED */
/* 574                              RESERVED */
/* 575                              RESERVED */
/* 576                              RESERVED */
/* 577                              RESERVED */

#define AVP_MAXARCHIVECACHE 578     /* Max gencahche size */
#define AVP_MESSAGE 579             /* Treat scanned object as a message */

/* 580                              RESERVED */
/* 581                              RESERVED */
/* 582                              RESERVED */
/* 583                              RESERVED */
/* 584                              RESERVED */

#define AVP_NOTESTFILES  585        /* Don't report test viruses */
#define AVP_COMPANIONVIRUSREPAIR 586
/* 587                              RESERVED */

#define AVP_MAILBOX      588        /* Scan mailboxes */

/* 589                              RESERVED */
#define AVP_UGFSTMPFILES 590
/* 591                              RESERVED */

#define AVP_DISABLE_TIMESLICES 592

/* 593                              RESERVED */
/* 594                              RESERVED */

#define AVP_CORRUPT 595             /* Attempt to handle corrupt Zip archive files */

/* 596                              RESERVED */

#define AVP_SHOWCOMPRESSED 597      /* Report files detected as ARCs (AVT_COMP) */

#define AVP_RAWOBJECTNAME 598       /* Send the AVM_OBJECTNAMERAW callbacks to report the raw object name */

#define AVP_USEDATSET 599           /* Specify a list of DAT sets that are to be used in the scan */
#define AVP_EXCLUDEDATSET 600       /* Specify a list of DAT sets to exclude from the scan */
#define AVP_NESTINGLIMIT 601       /* A limit on nesting levels scanned: default 15, max 300 */
/* 603                              RESERVED */
/* 604                              RESERVED */
#define AVP_REPAIRNOTIFICATION 605 /* Sign up for notification messages during repair */
/* 606                              RESERVED */
/* 607                              RESERVED */
#define AVP_UTF8OBJECTNAMES     609 /* Object names will be specified and reported using UTF-8 encoding */
/* 610                              RESERVED */
#define AVP_NODATOUTPUT         611 /* When performing a DAT update (AVUpdate), do not output any updated DAT set */
#define AVP_NODATCOMPRESS       612 /* When performing a DAT update (AVUpdate), do not compress the output DAT set. */
/* 613                              RESERVED */
/* 614                              RESERVED */
#define AVP_CONTAINER_LIST      615 /* List to the engine whether scan object has been de-composed */
/* 616                              RESERVED */
/* 617                              RESERVED */
#define AVP_FASTINITDATS        618 /* When performing a DAT update (AVUpdate), faster to initialise DATs (that are not compatible with 5300 and before). */
/* 619                              RESERVED */
#define AVP_UGFSWIN32FILEINFO   620 /* Windows specific extended file info callbacks are supported (WIN32LONGPATHNAME & WIN32FILEATTRIBUTES) */
#define AVP_BASE64RELAXED       621 /* Set Base64 decoder to relaxed mode. By default Base64 decoder adheres to Base64 standards. Relaxed mode mimics email client behavior. */
#define AVP_ENCRYPTEDDOCS       622 /* To scan encrypted document files */
/* 623                              RESERVED */
/* 624                              RESERVED */
#define AVP_UGFSWIN32FILEINFO2  625 /* Windows specific extended file info callbacks are supported (SETWIN32FILEATTRIBUTES) */
/* 626                              RESERVED */

#define AVP_ENDOFLIST 1000          /* Last parameter code in list - can be used as a loop terminator */

#endif

