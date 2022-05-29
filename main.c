#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json_tokener.h>
#include <json-c/json_object.h>
#include <ctype.h>
//#include <curl/curl.h> // replace system() command with c lib functions

#define MAIN 0
#define DEV 1
#define TEST 2


/*
 * ARGV 1 - file with wallet public key
 */


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

int main(int argc, char *argv[]) {
    // check if solana is installed
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
    char jsonBalanceRequestBuffer[200]; // string containing json file
    sprintf(jsonBalanceRequestBuffer,"{\"jsonrpc\":\"2.0\", \"id\":1, \"method\":\"getBalance\", \"params\":[\"%s\"]}",pubkey_string);
    FILE *jsonBalanceRequest = fopen("balanceRequest.json","w"); // json request file
    fprintf(jsonBalanceRequest,"%s",jsonBalanceRequestBuffer);
    fclose(jsonBalanceRequest);

    char get_balance_cmd[300] = {'\0'}; // curl request sent command
    sprintf(get_balance_cmd,"curl %s -X POST -H \"Content-Type: application/json\" -d \'%s\' > /tmp/solBalance.json",cluster_address,jsonBalanceRequestBuffer);

    char buffer[1024] = {'\0'};

    system(get_balance_cmd);
    FILE *jsonBalance = fopen("/tmp/solBalance.json","r");
    fread(buffer, 1024, 1, jsonBalance);
    fclose(jsonBalance);
    system("rm /tmp/solBalance.json");

    // read balance from required json file
    json_object *parsed_json = json_tokener_parse(buffer);
    sprintf(buffer,"%s",json_object_get_string(json_object_object_get(parsed_json, "result")));
    parsed_json = json_tokener_parse(buffer);
    char *ptr;
    long value = strtol(json_object_get_string(json_object_object_get(parsed_json, "value")),&ptr,10);
    double valueSol = (double)value/1000000000.0;
    printf("balance: %ld lamports   ( = %.5f SOL)\n",value,valueSol);

    return 0;
}
