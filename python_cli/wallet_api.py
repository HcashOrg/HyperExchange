# -*- coding: utf-8 -*-
import requests
import json
from base64 import encodestring
import time

class WalletApi:
    def __init__(self, name, conf):
        self.name = name
        self.config = conf

    def http_request(self, method, args):
        url = "http://%s:%s" % (self.config["host"], self.config["port"])
        user = 'a'
        passwd = 'b'
        basestr = encodestring('%s:%s' % (user, passwd))[:-1]
        args_j = json.dumps(args)
        payload =  "{\r\n \"id\": 1,\r\n \"method\": \"%s\",\r\n \"params\": %s\r\n}" % (method, args_j)
        #print payload
        headers = {
            'content-type': "text/plain",
            'authorization': "Basic %s" % (basestr),
            'cache-control': "no-cache",
        }
        response = requests.request("POST", url, data=payload, headers=headers)
        rep = response.json()
        return rep

time.localtime()
if __name__ == "__main__":
    config = {"host":"127.0.0.1","port":11086}

    wa = WalletApi("test",config)
    old_block = 0
    for j in range(98):
        while True:
            info_data = wa.http_request("getinfo",[])
            if info_data["result"]["blocks"] > old_block:
                old_block = info_data["result"]["blocks"]
                print "old_block",old_block
                break
            time.sleep(10)

        for i in range(10):
            unspent_data =  wa.http_request("listunspent",[0,99999,["LNnsz4kS9WG3wPsW9LPjYFhSYNXjjC4JDn"]])
            print unspent_data["result"]
            for one_data in unspent_data["result"]:
                txid = one_data["txid"]
                amount = one_data["amount"]
                address = one_data["address"]
                vout = one_data["vout"]
                print amount
                create_raw_transaction = wa.http_request("createrawtransaction",[[{"txid":txid,"vout":vout}],{"LNnsz4kS9WG3wPsW9LPjYFhSYNXjjC4JDn": "%.8f"%(amount - 10.0003),"LP3LrSdun3ZiZgeZn6nxEkyzKXqCAZsEJE":10}])
                print "create: ",create_raw_transaction
                sign_raw_transaction =  wa.http_request("signrawtransaction",[create_raw_transaction["result"]])
                print sign_raw_transaction
                broadcast = wa.http_request("sendrawtransaction",[sign_raw_transaction["result"]["hex"]])
                print broadcast
                time.sleep(0.5)


