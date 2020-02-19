#ifndef test_h__
#define test_h__

#include "gtest/gtest.h"

#define TESTUSER_USERNAME "UnitTests"
#define TESTUSER_PASSWORD "touch visit best though"

#define VAULT_TEST_SERVER "https://earth.vault.euclideon.com"
#define VAULT_TEST_MODEL_REMOTE (VAULT_TEST_SERVER "/models/Axis.uds")
#define VAULT_TEST_LOGO_REMOTE (VAULT_TEST_SERVER "/logo.png")
#define VAULT_TEST_MODEL_LOCAL "Axis.uds"

extern bool g_ignoreCertificateVerification;

struct vdkContext;
extern vdkContext *g_pSharedTestContext; // Most tests can use this 'shared' context with licenses already available rather than creating their own

#endif //test_h__
