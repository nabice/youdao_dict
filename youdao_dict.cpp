#include <QApplication>
#include <QClipboard>
#include <curl/curl.h>
#include <regex.h>
#include <iostream>
#include "json.h"
#include "json_util.h"
#include <signal.h>

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    char *result = (char *)userdata;
    size_t result_len = strlen(result);
    memcpy(&(result[result_len]), ptr, nmemb * size);
    result[result_len + size * nmemb] = '\0';
    return nmemb * size;
}
json_object *all_words;

void save_dict(__attribute__((__unused__)) int signum){
    json_object_to_file("/home/nabice/etc/youdao_db", all_words);
    exit(0);
}

class ClipboardProcess {
public:
    QClipboard::Mode mode;
    ClipboardProcess(QClipboard::Mode mode):mode(mode){
    }
    void operator()(){
        char url[200] = "http://dict.youdao.com/jsonapi?le=eng&dicts=%7B%22count%22%3A2%2C%22dicts%22%3A%5B%5B%22collins%22%5D%2C%5B%22ec21%22%5D%5D%7D&q=";
        char result[500000];
        char has_word = 0;
        result[0] = '\0';
        json_object *result_obj;
        json_object *origin_obj;
        json_object *value_obj = NULL;

        QClipboard * clipboard = QApplication::clipboard();
        const char *text = clipboard->text(mode).toUtf8().constData();
        int text_len = strlen(text);
        if (text_len > 60 || text_len < 1){
            return;
        }

        char word[61];
        //I don't know why, regcomp modify text's contents, so I copy it;
        memcpy(word, text, text_len + 1);
        const char * regexString = "[a-z]($|[a-z \\-]*[a-z])";
        regex_t regexCompiled;
        regcomp(&regexCompiled, regexString, REG_EXTENDED|REG_ICASE);
        regmatch_t matchptr[1];
        if (regexec(&regexCompiled, word, 1, matchptr, 0)){
            return;
        }
        char *text_tmp = &word[matchptr[0].rm_so];
        word[matchptr[0].rm_eo] = '\0';
        for(int i = 0; text_tmp[i]; i++){
            text_tmp[i] = tolower(text_tmp[i]);
        }
        regfree(&regexCompiled);
        if(json_object_object_get_ex(all_words, text_tmp, &value_obj)){
            result_obj = value_obj;
            has_word = 1;
        }else{
            has_word = 0;
            CURL *curl;
            strcat(url, text_tmp);
            curl = curl_easy_init();
            if(curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                result_obj = json_tokener_parse(result);
            }else{
                return;
            }
        }
        origin_obj = result_obj;
        if(json_object_object_get_ex(result_obj, "collins", &value_obj)){
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "collins_entries", &value_obj);
            value_obj = json_object_array_get_idx(value_obj, 0);
            result_obj = value_obj;
            if(json_object_object_get_ex(result_obj, "entries", &value_obj)){
                result_obj = value_obj;
                json_object_object_get_ex(result_obj, "entry", &value_obj);
                value_obj = json_object_array_get_idx(value_obj, 0);
                result_obj = value_obj;
                json_object_object_get_ex(result_obj, "tran_entry", &value_obj);
                value_obj = json_object_array_get_idx(value_obj, 0);
            }else if(json_object_object_get_ex(result_obj, "basic_entries", &value_obj)){
                result_obj = value_obj;
                json_object_object_get_ex(result_obj, "basic_entry", &value_obj);
                value_obj = json_object_array_get_idx(value_obj, 0);
            }
            result_obj = value_obj;
            if(json_object_object_get_ex(result_obj, "tran", &value_obj)){
                //value_obj is string
            }else if(json_object_object_get_ex(result_obj, "sees", &value_obj)){
                result_obj = value_obj;
                json_object_object_get_ex(result_obj, "see", &value_obj);
                value_obj = json_object_array_get_idx(value_obj, 0);
                result_obj = value_obj;
                json_object_object_get_ex(result_obj, "seeword", &value_obj);
            }
        }else if(json_object_object_get_ex(result_obj, "ec21", &value_obj)){
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "word", &value_obj);
            value_obj = json_object_array_get_idx(value_obj, 0);
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "trs", &value_obj);
            value_obj = json_object_array_get_idx(value_obj, 0);
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "tr", &value_obj);
            value_obj = json_object_array_get_idx(value_obj, 0);
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "l", &value_obj);
            result_obj = value_obj;
            json_object_object_get_ex(result_obj, "i", &value_obj);
            value_obj = json_object_array_get_idx(value_obj, 0);                                
        }
        if(json_type_string == json_object_get_type(value_obj)){
            FILE *f = fopen("/tmp/youdao_word", "wb");
            fwrite(text_tmp, 1, strlen(text_tmp), f);
            fclose(f);
            f = fopen("/tmp/youdao_result", "wb");
            const char *trans = json_object_get_string(value_obj);
            fwrite(trans, 1, strlen(trans), f);
            fclose(f);
            if(!has_word && strchr(text_tmp, ' ') == NULL){
                json_object_object_add(all_words, text_tmp, origin_obj);
            }
            if(system("/home/nabice/bin/del_youdao_result 2>/dev/null &")){
                //do nothing
            }
        }
    }
};

int main(int argc, char *argv[]){
    all_words = json_object_from_file("/home/nabice/etc/youdao_db");
    if(all_words == NULL){
        all_words = json_object_new_object();
    }
    QApplication app(argc, argv);
    ClipboardProcess clipboard(QClipboard::Clipboard);
    ClipboardProcess selection(QClipboard::Selection);
    QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, clipboard);
    QObject::connect(QApplication::clipboard(), &QClipboard::selectionChanged, selection);
    signal(SIGINT, save_dict);
    signal(SIGTERM, save_dict);
    signal(SIGHUP, save_dict);
    app.exec();
}
