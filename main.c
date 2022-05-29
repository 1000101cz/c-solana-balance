/* C Solana Balance
 *    - get actual solana wallet balance from solana cluster
 *
 * usage
 *    - './SolBalance /path/file_with_pubkey'
 *
 * requirements
 *    - json-c
 *    - curl c dev lib
 *
 */


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json_tokener.h>
#include <json-c/json_object.h>
#include <ctype.h>
#include <curl/curl.h>

#define MAIN 0
#define DEV 1
#define TEST 2


// check if any application is installed and in PATH
_Bool can_run_command(const char *cmd) {
    if(strchr(cmd, '/')) {
        return access(cmd, X_OK)==0;
    }
    const char *path = getenv("PATH");
    if(!path) return false;
    char *buf = malloc(strlen(path)+strlen(cmd)+3);
    if(!buf) return 0;
    for(; *path; ++path) {
        char *p = buf;
        for(; *path && *path!=':'; ++path,++p) {
            *p = *path;
        }
        if(p==buf) *p++='.';
        if(p[-1]!='/') *p++='/';
        strcpy(p, cmd);
        if(access(buf, X_OK)==0) {
            free(buf);
            return 1;
        }
        if(!*path) break;
    }
    free(buf);
    return 0;
}

// get response to balance request from Solana cluster
void curl_balance_request(char *cluster_address, char *json_request) {
    FILE *balance_json = fopen("/tmp/solBalance.json","wb");

    CURLcode ret;
    CURL *hnd;
    struct curl_slist *slist1;

    slist1 = NULL;
    slist1 = curl_slist_append(slist1, "Content-Type: application/json");

    hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(hnd, CURLOPT_URL, cluster_address);
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, json_request);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)107);
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, balance_json);

    ret = curl_easy_perform(hnd);

    curl_easy_cleanup(hnd);
    hnd = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;

    fclose(balance_json);
}

long get_balance(char *pubkey_string, char *cluster_address) {
    char jsonBalanceRequestBuffer[200]; // string containing json file
    sprintf(jsonBalanceRequestBuffer,"{\"jsonrpc\":\"2.0\", \"id\":1, \"method\":\"getBalance\", \"params\":[\"%s\"]}",pubkey_string);
    curl_balance_request(cluster_address,jsonBalanceRequestBuffer);

    // transform required json file to buffer
    char buffer[1024] = {'\0'};
    FILE *jsonBalance = fopen("/tmp/solBalance.json","r");
    fread(buffer, 1024, 1, jsonBalance);
    fclose(jsonBalance);
    system("rm /tmp/solBalance.json");

    // read balance data from buffer
    json_object *parsed_json = json_tokener_parse(buffer);
    sprintf(buffer,"%s",json_object_get_string(json_object_object_get(parsed_json, "result")));
    parsed_json = json_tokener_parse(buffer);
    char *ptr;

    return strtol(json_object_get_string(json_object_object_get(parsed_json, "value")),&ptr,10);
}

// convert lamports to SOL
double lam_to_sol(long valueLamports) {
    return (double)valueLamports/1000000000.0;
}

// convert SOL to lamports
long sol_to_lam(double valueSol) {
    return (long)(valueSol*1000000000.0);
}

int main(int argc, char *argv[]) {

    // check if solana is installed (not necessary yet)
    if (can_run_command("solana")) {
        system("solana --version");
    } else {
        printf("solana not installed or not found");
    }

    // select solana wallet
    char *pubkey_file;
    if (argc >= 2) {
        pubkey_file = argv[1];
    } else {
        fprintf(stderr,"enter pubkey file location\n");
        return 1;
    }

    // get public key string
    FILE *pubkey = fopen(pubkey_file,"r");
    char pubkey_string[45] = {'\0'};
    fread(pubkey_string, 50, 1, pubkey);
    fclose(pubkey);
    for (int i = 0; i < 45; i++) {
        if (!isdigit(pubkey_string[i]) && !isalpha(pubkey_string[i])) {
            pubkey_string[i] = '\0';
        }
    }
    printf("public key: %s\n",pubkey_string);

    // select Solana cluster
    int cluster = MAIN;
    char *cluster_address;
    switch (cluster) {
        case MAIN:
            cluster_address = "https://api.mainnet-beta.solana.com";
            break;

        case DEV:
            cluster_address = "https://api.devnet.solana.com";
            break;

        case TEST:
            cluster_address = "https://api.testnet.solana.com";
            break;

        default:
            fprintf(stderr, "cluster not set\n");
            return 1;
    }

    // get actual wallet balance from cluster
    long valueLamports = get_balance(pubkey_string, cluster_address);
    double valueSol = lam_to_sol(valueLamports);
    printf("\nWallet Balance:  %ld lamports   ( = %.5f SOL)\n",valueLamports,valueSol);

    return 0;
}
