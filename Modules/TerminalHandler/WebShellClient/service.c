// service.c -- Service descriptions

//#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "check.h"
#include "launcher.h"
#include "privileges.h"
#include "service.h"
#include "config.h"

#ifdef HAVE_UNUSED
#defined ATTR_UNUSED __attribute__((unused))
#defined UNUSED(x)   do { } while (0)
#else
#define ATTR_UNUSED
#define UNUSED(x)    do { (void)(x); } while (0)
#endif

struct Service **services;
int            numServices;

void initService(struct Service *service, const char *arg) {
  // The first part of the argument is the path where the service should
  // be mounted. Remove any trailing slashes and make sure there is exactly
  // one leading slash before copying it into service->path.
  char *desc;
  check(desc                                = strdup(arg));
  while (*arg == '/') {
    arg++;
  }
  char *ptr;
  if ((ptr = strchr(arg, ':')) == NULL) {
  error:
    fatal("Syntax error in service description \"%s\".", desc);
  }
  service->id                               = -1;
  check(service->path                       = malloc(ptr - arg + 2));
  ((char *)service->path)[0]                = '/';
  memcpy((char *)service->path + 1, arg, ptr - arg);
  ((char *)service->path)[ptr - arg + 1]    = '\000';
  while (service->path[1] && strrchr(service->path, '\000')[-1] == '/') {
    strrchr(service->path, '\000')[-1]      = '\000';
  }
  arg                                       = ptr + 1;

#ifdef HAVE_BIN_LOGIN
  // The next part of the argument is either the word 'LOGIN' or the
  // application definition.
  if (!strcmp(arg, "LOGIN")) {
    if (geteuid()) {
      fatal("Must be \"root\" to invoke \"/bin/login\". Maybe, change "
            "--service definitions?");
    }
    service->useLogin                       = 1;
    service->useHomeDir                     = 0;
    service->authUser                       = 0;
    service->useDefaultShell                = 0;
    service->uid                            = 0;
    service->gid                            = 0;
    check(service->user                     = strdup("root"));
    check(service->group                    = strdup("root"));
    check(service->cwd                      = strdup("/"));
    check(service->cmdline                  = strdup(
                                                  "/bin/login -p -h ${peer}"));
  } else
#endif
  if (!strcmp(arg, "SSH") || !strncmp(arg, "SSH:", 4)) {
    service->useLogin                       = 0;
    service->useHomeDir                     = 0;
    service->authUser                       = 2;
    service->useDefaultShell                = 0;
    service->uid                            = -1;
    service->gid                            = -1;
    service->user                           = NULL;
    service->group                          = NULL;
    check(service->cwd                      = strdup("/"));
    char *host;
    check(host                              = strdup("localhost"));
    if ((ptr                                = strchr(arg, ':')) != NULL) {
      check(ptr                             = strdup(ptr + 1));
      char *end;
      if ((end                              = strchr(ptr, ':')) != NULL) {
        *end                                = '\000';
      }
      if (*ptr) {
        free(host);
        host                                = ptr;
      } else {
        free(ptr);
      }
    }

    // Don't allow manipulation of the SSH command line through "creative" use
    // of the host name.
    char *h = host;
    for (; *h; h++) {
      char ch                               = *h;
      if (!((ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            ch == '-' || ch == '.')) {
        fatal("Invalid hostname \"%s\" in service definition", host);
      }
    }

    service->cmdline                        = stringPrintf(NULL,
      "ssh -a -e none -i /dev/null -x -oChallengeResponseAuthentication=no "
          "-oCheckHostIP=no -oClearAllForwardings=yes -oCompression=no "
          "-oControlMaster=no -oGSSAPIAuthentication=no "
          "-oHostbasedAuthentication=no -oIdentitiesOnly=yes "
          "-oKbdInteractiveAuthentication=yes -oPasswordAuthentication=yes "
          "-oPreferredAuthentications=keyboard-interactive,password "
          "-oPubkeyAuthentication=no -oRhostsRSAAuthentication=no "
          "-oRSAAuthentication=no -oStrictHostKeyChecking=no -oTunnel=no "
          "-oUserKnownHostsFile=/dev/null -oVerifyHostKeyDNS=no "
// beewoolie-2012.03.30: while it would be nice to disable this
//          feature, we cannot be sure that it is available on the
//          target server.  Removing it for the sake of Centos.
//          "-oVisualHostKey=no"
	  " -oLogLevel=QUIET %%s@%s", host);
    free(host);
  } else {
    service->useLogin                       = 0;

    // The user definition is either the word 'AUTH' or a valid user and
    // group id.
    if ((ptr                                = strchr(arg, ':')) == NULL) {
      goto error;
    }
    *ptr                                    = '\000';
    if (supportsPAM() && !strcmp(arg, "AUTH")) {
      service->authUser                     = 1;
      service->uid                          = -1;
      service->gid                          = -1;
      service->user                         = NULL;
      service->group                        = NULL;
    } else {
      service->authUser                     = 0;

      // Numeric or symbolic user id
      service->uid                          = parseUserArg(arg,
                                                           &service->user);
      *ptr                                  = ':';
      arg                                   = ptr + 1;

      // Numeric or symbolic group id
      if ((ptr                              = strchr(arg, ':')) == NULL) {
        goto error;
      }
      *ptr                                  = '\000';
      service->gid                          = parseGroupArg(arg,
                                                            &service->group);
    }
    *ptr                                    = ':';
    arg                                     = ptr + 1;

    // The next part of the argument is the starting working directory
    if ((ptr                                = strchr(arg, ':')) == NULL) {
      goto error;
    }
    *ptr                                    = '\000';
    if (!strcmp(arg, "HOME")) {
      service->useHomeDir                   = 1;
      service->cwd                          = NULL;
    } else {
      if (*arg != '/') {
        fatal("Working directories must have absolute paths");
      }
      service->useHomeDir                   = 0;
      check(service->cwd                    = strdup(arg));
    }
    *ptr                                    = ':';
    arg                                     = ptr + 1;

    // The final argument is the command line
    if (!*arg) {
      goto error;
    }
    if (!strcmp(arg, "SHELL")) {
      service->useDefaultShell              = 1;
      service->cmdline                      = NULL;
    } else {
      service->useDefaultShell              = 0;
      check(service->cmdline                = strdup(arg));
    }
  }
  free(desc);
}

struct Service *newService(const char *arg) {
  struct Service *service;
  check(service = malloc(sizeof(struct Service)));
  printf("newService:%s\n", arg);
  initService(service, arg);
  return service;
}

void destroyService(struct Service *service) {
  if (service) {
    free((char *)service->path);
    free((char *)service->user);
    free((char *)service->group);
    free((char *)service->cwd);
    free((char *)service->cmdline);
  }
}

void deleteService(struct Service *service) {
  int i, allRelease = 1;

  // search service and mark invalid
  for (i = 0; i < numServices; i++) {
	  if (services[i] == service) {
		  services[i] = NULL;
	  }
  }

  destroyService(service);
  free(service);

  // check if all services are release
  for (i = 0; i < numServices; i++) {
	  if (services[i]) {
		  allRelease = 0;
	  }
  }
  if (allRelease) {
	  free(services);
  }
}

void destroyServiceHashEntry(void *arg ATTR_UNUSED, char *key ATTR_UNUSED,
                             char *value ATTR_UNUSED) {
  UNUSED(arg);
  UNUSED(key);
  UNUSED(value);
}

static int enumerateServicesHelper(void *arg ATTR_UNUSED,
                                   const char *key ATTR_UNUSED, char **value) {
  UNUSED(arg);
  UNUSED(key);

  check(services              = realloc(services,
                                    ++numServices * sizeof(struct Service *)));
  services[numServices-1]     = *(struct Service **)value;
  services[numServices-1]->id = numServices-1;
  return 1;
}

void enumerateServices(struct HashMap *serviceTable) {
  check(!services);
  check(!numServices);
  iterateOverHashMap(serviceTable, enumerateServicesHelper, NULL);
}
