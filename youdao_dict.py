#!/usr/bin/env python
# -*- coding: utf-8 -*-
import json
import sys
import os
import urllib
import urllib2

def print_sees(sees):
    print "-> see:", ", ".join([x["seeword"] for x in sees["see"]])

def query(word):
    dict_path = "/home/nabice/etc/youdao_db"
    if os.path.exists(dict_path):
        all_words = json.loads(open(dict_path).read())
    else:
        all_words = {}
    if word in all_words:
        result = all_words[word]
    else:
        result = json.loads(urllib2.urlopen('http://dict.youdao.com/jsonapi?' + urllib.urlencode({"nocache":"true", "le":"eng", "q":word, "dicts":'{"count":2,"dicts":[["collins"]]}'})).read())
    if "collins" in result:
        for collins_entry in result["collins"]["collins_entries"]:
            if "super_headword" in collins_entry:
                print "===========[" + collins_entry["super_headword"]+ "]============"
            for basic_entry in collins_entry["basic_entries"]:
                if "sees" in basic_entry:
                    print_sees(basic_entry["sees"])
            if "entries" in collins_entry:
                i = 0
                for entry in collins_entry["entries"]["entry"]:
                    i = i + 1
                    print str(i) + ":", 
                    for tran_entry in entry["tran_entry"]:
                        if "pos_entry" in tran_entry:
                            if "pos_tips" in tran_entry["pos_entry"]:
                                print tran_entry["pos_entry"]["pos_tips"].encode("utf8")
                            elif "pos" in tran_entry["pos_entry"]:
                                print tran_entry["pos_entry"]["pos"]
                        if "headword" in tran_entry:
                            print tran_entry["headword"],
                        if "tran" in tran_entry:
                            print tran_entry["tran"].encode("utf8")
                        if "gram" in tran_entry:
                            print "["+tran_entry["gram"]+"]"
                        if "seeAlsos" in tran_entry:
                            print tran_entry["seeAlsos"]["seealso"], ":", ", ".join([x["seeword"] for x in tran_entry["seeAlsos"]["seeAlso"]])
                        if "sees" in tran_entry:
                            print_sees(tran_entry["sees"])
                        if "exam_sents" in tran_entry:
                            for sent in tran_entry["exam_sents"]["sent"]:
                                print "   例：", sent["eng_sent"]
                                print "        ", sent["chn_sent"].encode("utf8")
                        print
                print
    else:
        print "Not Found"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        result = query(sys.argv[1])
