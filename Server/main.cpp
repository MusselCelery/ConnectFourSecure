#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/x509_vfy.h>
#include <openssl/err.h> // for error descriptions

#define PORT 8080

using namespace std;

void checkCertificate(){
  int ret; // used for return values

   // load the CA's certificate:
   string cacert_file_name;
   cout << "Please, type the PEM file containing a trusted CA's certificate: ";
   getline(cin, cacert_file_name);
   if(!cin) { cerr << "Error during input\n"; return;}
   FILE* cacert_file = fopen(cacert_file_name.c_str(), "r");
   if(!cacert_file){ cerr << "Error: cannot open file '" << cacert_file_name << "' (missing?)\n"; return;}
   X509* cacert = PEM_read_X509(cacert_file, NULL, NULL, NULL);
   fclose(cacert_file);
   if(!cacert){ cerr << "Error: PEM_read_X509 returned NULL\n"; return; }

   // load the CRL:
   string crl_file_name;
   cout << "Please, type the PEM file containing a trusted CRL: ";
   getline(cin, crl_file_name);
   if(!cin) { cerr << "Error during input\n"; return; }
   FILE* crl_file = fopen(crl_file_name.c_str(), "r");
   if(!crl_file){ cerr << "Error: cannot open file '" << crl_file_name << "' (missing?)\n"; return; }
   X509_CRL* crl = PEM_read_X509_CRL(crl_file, NULL, NULL, NULL);
   fclose(crl_file);
   if(!crl){ cerr << "Error: PEM_read_X509_CRL returned NULL\n"; return; }

   // build a store with the CA's certificate and the CRL:
   X509_STORE* store = X509_STORE_new();
   if(!store) { cerr << "Error: X509_STORE_new returned NULL\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }
   ret = X509_STORE_add_cert(store, cacert);
   if(ret != 1) { cerr << "Error: X509_STORE_add_cert returned " << ret << "\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }
   ret = X509_STORE_add_crl(store, crl);
   if(ret != 1) { cerr << "Error: X509_STORE_add_crl returned " << ret << "\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }
   ret = X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK);
   if(ret != 1) { cerr << "Error: X509_STORE_set_flags returned " << ret << "\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }

   // load the peer's certificate:
   string cert_file_name;
   cout << "Please, type the PEM file containing peer's certificate: ";
   getline(cin, cert_file_name);
   if(!cin) { cerr << "Error during input\n"; return; }
   FILE* cert_file = fopen(cert_file_name.c_str(), "r");
   if(!cert_file){ cerr << "Error: cannot open file '" << cert_file_name << "' (missing?)\n"; return; }
   X509* cert = PEM_read_X509(cert_file, NULL, NULL, NULL);
   fclose(cert_file);
   if(!cert){ cerr << "Error: PEM_read_X509 returned NULL\n"; return; }

   // verify the certificate:
   X509_STORE_CTX* certvfy_ctx = X509_STORE_CTX_new();
   if(!certvfy_ctx) { cerr << "Error: X509_STORE_CTX_new returned NULL\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }
   ret = X509_STORE_CTX_init(certvfy_ctx, store, cert, NULL);
   if(ret != 1) { cerr << "Error: X509_STORE_CTX_init returned " << ret << "\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }
   ret = X509_verify_cert(certvfy_ctx);
   if(ret != 1) { cerr << "Error: X509_verify_cert returned " << ret << "\n" << ERR_error_string(ERR_get_error(), NULL) << "\n"; return; }

   // print the successful verification to screen:
   char* tmp = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
   char* tmp2 = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
   cout << "Certificate of \"" << tmp << "\" (released by \"" << tmp2 << "\") verified successfully\n";
   free(tmp);
   free(tmp2);

   // load the signature file:
   string sgnt_file_name;
   cout << "Please, type the signature file: ";
   getline(cin, sgnt_file_name);
   if(!cin) { cerr << "Error during input\n"; return; }
   FILE* sgnt_file = fopen(sgnt_file_name.c_str(), "rb");
   if(!sgnt_file) { cerr << "Error: cannot open file '" << sgnt_file_name << "' (file does not exist?)\n"; return; }

   // get the file size:
   // (assuming no failures in fseek() and ftell())
   fseek(sgnt_file, 0, SEEK_END);
   long int sgnt_size = ftell(sgnt_file);
   fseek(sgnt_file, 0, SEEK_SET);

   // read the signature from file:
   unsigned char* sgnt_buf = (unsigned char*)malloc(sgnt_size);
   if(!sgnt_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; return; }
   ret = fread(sgnt_buf, 1, sgnt_size, sgnt_file);
   if(ret < sgnt_size) { cerr << "Error while reading file '" << sgnt_file_name << "'\n"; return; }
   fclose(sgnt_file);

   // declare some useful variables:
   const EVP_MD* md = EVP_sha256();
   const char clear_buf[] = "Hello, peer!\n";
   int clear_size = strlen(clear_buf);

   // create the signature context:
   EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
   if(!md_ctx){ cerr << "Error: EVP_MD_CTX_new returned NULL\n"; return; }

   // verify the plaintext:
   // (perform a single update on the whole plaintext,
   // assuming that the plaintext is not huge)
   ret = EVP_VerifyInit(md_ctx, md);
   if(ret == 0){ cerr << "Error: EVP_VerifyInit returned " << ret << "\n"; return; }
   ret = EVP_VerifyUpdate(md_ctx, clear_buf, clear_size);
   if(ret == 0){ cerr << "Error: EVP_VerifyUpdate returned " << ret << "\n"; return; }
   ret = EVP_VerifyFinal(md_ctx, sgnt_buf, sgnt_size, X509_get_pubkey(cert));
   if(ret == -1){ // it is 0 if invalid signature, -1 if some other error, 1 if success.
      cerr << "Error: EVP_VerifyFinal returned " << ret << " (invalid signature?)\n";
      return;
   }else if(ret == 0){
      cerr << "Error: Invalid signature!\n";
      return;
   }

   // print the successful signature verification to screen:
   cout << "The subject really said \"" << "Hello, peer!\n" << "\"\n";

   // deallocate data:
   EVP_MD_CTX_free(md_ctx);
   X509_free(cert);
   X509_STORE_free(store);
   //X509_free(cacert); // already deallocated by X509_STORE_free()
   //X509_CRL_free(crl); // already deallocated by X509_STORE_free()
   X509_STORE_CTX_free(certvfy_ctx);
}


int main(int argc, char const *argv[])
{
    checkCertificate();

    cin.get();
    cin.get();

    return 0;
}
