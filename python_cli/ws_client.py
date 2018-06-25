from ws4py.client.threadedclient import WebSocketClient
import json


message_list = []
def call_function(id,method_name,params):
    method = '{"id":%d,"method":"%s","params":%s}'
    params_str = json.dumps(params)
    method = method %(id,method_name,params_str)
    return method

class DummyClient(WebSocketClient):
    def opened(self):
        call_method = call_function(4, "call", [1,"login",["bytemaster","12345678"]])
        self.send(call_method)



    def closed(self, code, reason=None):
        print "Closed down", code, reason

    def received_message(self, m):
        print m
        m = str(m).replace('null','"null"')
        json_obj = json.loads(m)

        print json_obj
        if json_obj["id"] == 0:
            print "success"
            call_method = call_function(1, "info", [])
            self.send(call_method)
        if json_obj["id"] == 1:
            print ""
            print ""
            print ""

            call_method = call_function(2, "call", [0,"info",[]])
            self.send(call_method)
        if json_obj["id"] ==2:
            call_method = call_function(3, "call", [0, "login", ["*","*"]])
            self.send(call_method)

        if json_obj["id"] == 4:
            call_method = call_function(5, "call", [1, "network_node", []])
            self.send(call_method)
        if json_obj["id"] == 5:
            call_method = call_function(99, "call", [2, "help", []])
            self.send(call_method)
        if json_obj["id"] == 99:
            self.close(reason ="bye bye")

if __name__ == '__main__':
    try:
        ws = DummyClient('ws://127.0.0.1:8091', protocols=['chat'])
        ws.connect()
        ws.run_forever()

    except KeyboardInterrupt:
        ws.close()