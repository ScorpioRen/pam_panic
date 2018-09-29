/*
FILENAME :     test.c
DESCRIPTION :  Test suites with CUnit for pam_panic
AUTHOR :       Bandie
DATE :         2018-05-11T04:13:51+02:00
LICENSE :      GNU-GPLv3
*/

#include <stdint.h>
#include <unistd.h>
#include <security/pam_modules.h>

#include "../lib/gettext.h"
#include "../src/pam_panic/pam_panic_authdevice.h"
#include "../src/pam_panic/pam_panic_reject.h"
#include "../src/pam_panic_pw/pam_panic_pw.h"
#include <CUnit/Basic.h>

#define STATE_GOOD 0
#define STATE_BAD 99
#define STATE_NA 1

#define STATE_REJ_SER 0
#define STATE_REJ_REB 1
#define STATE_REJ_POW 2
#define STATE_REJ_NA 3

#define STATE_PPP_WRITEPASSWORDS 0

#define GOODUUID "./good"
#define BADUUID  "./bad"

#define PASSWORDFILE "./pwfile"
#define GOODPASSWORD "yip"
#define BADPASSWORD "69"

char* gU = GOODUUID;
char* bU = BADUUID;

int init_suite(void) {
  return 0;
}

int clean_suite(void) {
  return 0;
}


// pam_panic_authdevice tests
void test_authDeviceGood(void) {
  FILE *f = fopen(gU, "w");
  fclose(f);

  int ret = authDevice(NULL, gU, bU, NULL, 0, 0, 0);
  CU_ASSERT_EQUAL(ret, STATE_GOOD);
  unlink(gU);
}

void test_authDeviceBad(void) {
  FILE *f = fopen(bU, "w");
  fclose(f);
  
  int ret = authDevice(NULL, gU, bU, NULL, 0, 0, 0);
  CU_ASSERT_EQUAL(ret, STATE_BAD);
  unlink(bU);
}

void test_authDeviceNA(void) {
  int ret = authDevice(NULL, gU, bU, NULL, 0, 0, 0);
  CU_ASSERT_EQUAL(ret, STATE_NA);
}


// pam_panic_reject tests
void test_rejectSerious(void) {
  int ret = reject(NULL, 1, 0, 0);
  CU_ASSERT_EQUAL(ret, STATE_REJ_SER); 
}

void test_rejectReboot(void) {
  int ret = reject(NULL, 0, 1, 0);
  CU_ASSERT_EQUAL(ret, STATE_REJ_REB);
}

void test_rejectPoweroff(void) {
  int ret = reject(NULL, 0, 0, 1);
  CU_ASSERT_EQUAL(ret, STATE_REJ_POW);
}

void test_rejectNA(void) {
  int ret = reject(NULL, 0, 0, 0);
  CU_ASSERT_EQUAL(ret, STATE_REJ_NA);
}


// pam_panic_pw tests
void test_writePassword(void) {

  char encpw[2][99];

  strcpy(encpw[0], crypt(GOODPASSWORD, "$6$somesalt"));
  strcpy(encpw[1], crypt(BADPASSWORD, "$6$somesalt"));

  int ret = writePasswords(encpw, PASSWORDFILE);
  CU_ASSERT_EQUAL(ret, STATE_PPP_WRITEPASSWORDS);
}

void test_passwordCheckFromFile(void) {

  int ret;
  char buf[2][256];
  char* line = NULL;

  FILE *f = fopen(PASSWORDFILE, "r");
  size_t len = 0;
  ssize_t read;

  if(f){
    int i = 0;
    while(read = getline(&line, &len, f)){
      strncpy(buf[i], line, len);
      if(++i > 1)
        break;
    }
    if(ferror(f))
      CU_FAIL("Some error occured with the password file.");
    fclose(f);
  }


  ret = strcmp(strtok(buf[0], "\n"), crypt(GOODPASSWORD, buf[0])) == 0;
  CU_ASSERT_TRUE(ret);

  ret = strcmp(strtok(buf[1], "\n"), crypt(BADPASSWORD, buf[1])) == 0;
  CU_ASSERT_TRUE(ret);

}


int main(void) {

  // no stdout buffering
  //setbuf(stdout, NULL);
  
  // init CUnit test registry
  CU_pSuite pSuiteDevice = NULL;
  CU_pSuite pSuiteReject = NULL;
  CU_pSuite pSuitePasswordWrite = NULL;
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  // Make suits
  pSuiteDevice = CU_add_suite("Suite pam_panic_authdevice", init_suite, clean_suite);
  pSuiteReject = CU_add_suite("Suite pam_panic_reject", init_suite, clean_suite);
  pSuitePasswordWrite = CU_add_suite("Suite pam_panic_pw", init_suite, clean_suite);
  if (pSuiteDevice == NULL
    || pSuiteReject == NULL || pSuitePasswordWrite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  // adding tests to all suits
          // SuiteDevice
  if (   (NULL == CU_add_test(pSuiteDevice, "Authenticate with good device?", test_authDeviceGood))
      || (NULL == CU_add_test(pSuiteDevice, "Authenticate with bad device?", test_authDeviceBad))
      || (NULL == CU_add_test(pSuiteDevice, "Authenticate with no device?", test_authDeviceNA))
      || (NULL == CU_add_test(pSuiteReject, "Reject: Serious?", test_rejectSerious))
      || (NULL == CU_add_test(pSuiteReject, "Reject: Reboot?", test_rejectReboot))
      || (NULL == CU_add_test(pSuiteReject, "Reject: Poweroff?", test_rejectPoweroff))
      || (NULL == CU_add_test(pSuiteReject, "Reject: Nothing?", test_rejectNA))
      || (NULL == CU_add_test(pSuitePasswordWrite, "pam_panic_pw: Write password?", test_writePassword))
      || (NULL == CU_add_test(pSuitePasswordWrite, "pam_panic_pw: Passwords from file work?", test_passwordCheckFromFile))
     ) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Run all tests */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  
  int fail = CU_get_number_of_failures();

  CU_cleanup_registry();

  if(fail)
    return 1;

  return CU_get_error();
}

