#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include "uthash.h"

struct MemoryBuffer {
    char *data;
    size_t size;
};

typedef struct EmissionEntry {
    char date[11]; // YYYY-MM-DD + null terminator
    double gold;
    int nft_count;
    UT_hash_handle hh;
} EmissionEntry;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
char *build_url(const char *wallet_id);
CURL *init_curl(const char *url, struct MemoryBuffer *buffer);
json_t *parse_json_response(const char *data);
int extract_nft_count(json_t *root);
int write_emissions_to_file(EmissionEntry *emissions, const char *filename);
EmissionEntry *load_existing_emissions(const char *filename);
EmissionEntry *merge_emissions(EmissionEntry *emissions, json_t *root, int nft_count);

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

    EmissionEntry *emissions = load_existing_emissions("emissions.txt");
    emissions = merge_emissions(emissions, root, nft_count);

    if (write_emissions_to_file(emissions, "emissions.txt") != 0) {
        return EXIT_FAILURE;
    }

    curl_easy_cleanup(curl);

    EmissionEntry *e, *tmp;
    HASH_ITER(hh, emissions, e, tmp) {
        HASH_DEL(emissions, e);
        free(e);
    }

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
 * sort_by_date() is used to sort the hash entries into ascending
 * order by date, useful to use before writing out to the file.
 */
int sort_by_date(const EmissionEntry *a, const EmissionEntry *b) {
    return strcmp(a->date, b->date);
}

/*
 * write_emissions_to_file() parses the root object for dailyEmissions and
 * attempts to write the relevant values down to the provided filename, it 
 * will return -1 if an error occurs at any stage.
 * file format: date value nft_count
 */
int write_emissions_to_file(EmissionEntry *emissions, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    HASH_SORT(emissions, sort_by_date);

    EmissionEntry *entry, *tmp;
    HASH_ITER(hh, emissions, entry, tmp) {
        fprintf(fp, "%s %.2f %d\n", entry->date, entry->gold, entry->nft_count);
    }

    fclose(fp);
    return 0;
}

/*
 * load_existing_emissions() attempts to load the data within a  
 * provided filename into the EmissionEntry hash table, if it 
 * is unable to read the file, or deal with parsing the file 
 * then NULL is returned.
 */
EmissionEntry *load_existing_emissions(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        // File may not exist (first run), treat as empty.
        return NULL;
    }

    EmissionEntry *emissions = NULL;

    char line[128];

    // Skip the header row.
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        char date[11];
        double gold;
        int nft_count;

        int matched = sscanf(line, "%10s %lf %d", date, &gold, &nft_count);
        if (matched != 3) {
            fprintf(stderr, "skipping malformed line: %s", line);
            continue;
        }

        EmissionEntry *entry = malloc(sizeof(EmissionEntry));
        if (!entry) {
            perror("malloc");
            fclose(fp);
            return NULL;
        }

        strncpy(entry->date, date, 11);
        entry->gold = gold;
        entry->nft_count = nft_count;

        HASH_ADD_STR(emissions, date, entry);
    }

    fclose(fp);

    return emissions;
}

EmissionEntry *merge_emissions(EmissionEntry *emissions, json_t *root, int nft_count) {
    json_t *daily_emissions = json_object_get(root, "dailyEmissions");
    if (!json_is_object(daily_emissions)) {
        fprintf(stderr, "daily_emissions was not an object, cannot parse\n");
        return NULL;
    }

    const char *date; 
    json_t *value;

    json_object_foreach(daily_emissions, date, value) {
        double gold = 0.0;

        if (json_is_real(value)) {
            gold = json_real_value(value);
        } else if (json_is_integer(value)) {
            gold = (double) json_integer_value(value);
        } else {
            fprintf(stderr, "unexpected value type for key %s\n", date);
            continue;
        }

        EmissionEntry *entry;
        HASH_FIND_STR(emissions, date, entry);

        // New Entry, create and store.
        if (!entry) {
            entry = malloc(sizeof(EmissionEntry));
            if (!entry) {
                perror("malloc");
                continue;
            }

            strncpy(entry->date, date, 11);
            entry->gold = gold;
            entry->nft_count = nft_count;

            HASH_ADD_STR(emissions, date, entry);
            continue;
        }

        if (gold > entry->gold) {
            entry->gold = gold;
        }

        if (nft_count > entry->nft_count) {
            entry->nft_count = nft_count;
        }
    }

    return emissions;
}
