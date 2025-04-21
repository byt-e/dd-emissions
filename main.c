#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

struct MemoryBuffer {
    char *data;
    size_t size;
};

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

int main(void) {
    CURL *curl;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "could not init curl\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, 
        "https://nightvale-insight.xyz/api/stats/wallet?wallet=AMsaQ2vgSDseRa28cgBxiemdMzNUJhas3882dnmm6xeh");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    struct MemoryBuffer chunk;
    chunk.data = NULL;
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    CURLcode resp = curl_easy_perform(curl);
    if (resp != CURLE_OK) {
        fprintf(stderr, "curl request failed: %s\n", curl_easy_strerror(resp));
        return 1;
    }

    json_t *root;
    json_error_t error;

    root = json_loads(chunk.data, 0, &error);
    free(chunk.data);

    if (!root) {
        fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
        return 1;
    }

    json_t *nfts = json_object_get(root, "nfts");
    if (!json_is_object(nfts)) {
        fprintf(stderr, "nfts was not an object, cannot parse\n");
        return 1;
    }

    json_t *nft_count;
    nft_count = json_object_get(nfts, "nftCount");

    if (!json_is_integer(nft_count)) {
        fprintf(stderr, "nft count was not an integer\n");
        return 1;
    }

    json_t *daily_emissions = json_object_get(root, "dailyEmissions");
    if (!json_is_object(daily_emissions)) {
        fprintf(stderr, "daily_emissions was not an object, cannot parse\n");
        return 1;
    }


    const char *key;
    json_t *value;


    FILE *fp = fopen("emissions.txt", "w");
    fprintf(fp, "date value nft_count\n");

    double gold;
    json_object_foreach(daily_emissions, key, value) {
        if (json_is_real(value)) {
            gold = json_real_value(value);
        } else if (json_is_integer(value)) {
            gold = (double) json_integer_value(value);
        } else {
            fprintf(stderr, "unexpected value type for key %s\n", key);
            continue;
        }

        fprintf(fp, "%s %.2f %d\n", key, gold, (int) json_integer_value(nft_count));
    }

    fclose(fp);

    curl_easy_cleanup(curl);

    return 0;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    size_t data_size = size * nmemb;
    
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;

    char *ptr = realloc(mem->data, (mem->size + data_size + 1) * sizeof(char));
    if (!ptr) {
        fprintf(stderr, "not enough memory (realloac returned NULL)\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), buffer, data_size);
    mem->size += data_size;
    mem->data[mem->size] = 0;

    return data_size;
}
