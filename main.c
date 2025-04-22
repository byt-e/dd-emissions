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
char *build_url(const char *wallet_id);
CURL *init_curl(const char *url, struct MemoryBuffer *buffer);
json_t *parse_json_response(const char *data);
int extract_nft_count(json_t *root);
int write_emissions_to_file(json_t *root, int nft_count, const char *filename);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <wallet-id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *wallet_id = argv[1];

    struct MemoryBuffer chunk;
    chunk.data = NULL;
    chunk.size = 0;

    CURL *curl = init_curl(wallet_id, &chunk);
    if (curl == NULL) {
        fprintf(stderr, "could not initialise curl handle\n");
        return EXIT_FAILURE;
    }

    CURLcode resp = curl_easy_perform(curl);
    if (resp != CURLE_OK) {
        fprintf(stderr, "curl request failed: %s\n", curl_easy_strerror(resp));
        return EXIT_FAILURE;
    }

    json_t *root = parse_json_response(chunk.data);
    free(chunk.data);

    if (root == NULL) {
        fprintf(stderr, "could not parse json response\n");
        return EXIT_FAILURE;
    }

    int nft_count = extract_nft_count(root);
    if (nft_count < 0) {
        return EXIT_FAILURE;
    }

    if (write_emissions_to_file(root, nft_count, "emissions.txt") != 0) {
        return EXIT_FAILURE;
    }

    curl_easy_cleanup(curl);

    return EXIT_SUCCESS;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    size_t data_size = size * nmemb;
    
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;

    char *ptr = realloc(mem->data, (mem->size + data_size + 1) * sizeof(char));
    if (!ptr) {
        fprintf(stderr, "not enough memory (realloac returned NULL)\n");
        return EXIT_FAILURE;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), buffer, data_size);
    mem->size += data_size;
    mem->data[mem->size] = 0;

    return data_size;
}

/*
 * build_url() constructs the full nightvale url with the provided wallet_id, 
 * storing it safely onto the heap and returning NULL if there is not enough 
 * available memory to allocate, or if the wallet_id exceeds the expected
 * buffer size.
 */
char *build_url(const char *wallet_id) {
    const char *base_url = "https://nightvale-insight.xyz/api/stats/wallet?wallet=";

    size_t len = strlen(base_url) + strlen(wallet_id) + 1;
    
    char *url = malloc(len);
    if (url == NULL) {
        fprintf(stderr, "failed to allocate memory for URL\n");
        return NULL;
    }

    snprintf(url, len, "%s%s", base_url, wallet_id);

    return url;
}

/*
 * init_curl() constructs a curl handle with the relevant fields for 
 * processing the HTTP request and parsing the response into the
 * provided memory buffer.
*/
CURL *init_curl(const char *wallet_id, struct MemoryBuffer *buffer) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "could not initialise curl handle\n");
        return NULL;
    }

    char *url = build_url(wallet_id);
    if (url == NULL) {
        fprintf(stderr, "could not build url for curl handle\n");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    free(url);

    return curl;
}

/*
* parse_json_response() takes a raw JSON string and attempts to 
* parse it using Jansson. If parsing fails, it returns NULL back 
* to the caller.
*/
json_t *parse_json_response(const char *data) {
    json_error_t error;
    json_t *root = json_loads(data, 0, &error);

    if (!root) {
        fprintf(stderr, "could not parse json data\n");
        return NULL;
    }

    return root;
}

/*
 * extract_nft_count() takes in a root JSON node and searches the
 * tree for the expected nfts object, and extracts out the nft
 * count, failing and returning -1 if neither are present.
*/
int extract_nft_count(json_t *root) {
    json_t *nfts = json_object_get(root, "nfts");
    if (!json_is_object(nfts)) {
        fprintf(stderr, "nfts was not an object, cannot parse\n");
        return -1;
    }

    json_t *nft_count = json_object_get(nfts, "nftCount");
    if (!json_is_integer(nft_count)) {
        fprintf(stderr, "nft count was not an integer\n");
        return -1;
    }

    return (int) json_integer_value(nft_count);
}

/*
 * write_emissions_to_file() parses the root object for dailyEmissions and
 * attempts to write the relevant values down to the provided filename, it 
 * will return -1 if an error occurs at any stage.
 * file format: date value nft_count
 */
int write_emissions_to_file(json_t *root, int nft_count, const char *filename) {
    json_t *daily_emissions = json_object_get(root, "dailyEmissions");
    if (!json_is_object(daily_emissions)) {
        fprintf(stderr, "daily_emissions was not an object, cannot parse\n");
        return -1;
    }

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    fprintf(fp, "date value nft_count\n");

    const char *key;
    json_t *value;

    json_object_foreach(daily_emissions, key, value) {
        double gold = 0.0;

        if (json_is_real(value)) {
            gold = json_real_value(value);
        } else if (json_is_integer(value)) {
            gold = (double) json_integer_value(value);
        } else {
            fprintf(stderr, "unexpected value type for key %s\n", key);
            continue;
        }

        fprintf(fp, "%s %.2f %d\n", key, gold, nft_count);
    }

    fclose(fp);

    return 0;
}
