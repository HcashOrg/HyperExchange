The bitshares developer community use different ways to interact with the core api mainly by the use of the websocket.

Some of this methods are curl, pybitshares, wscat and many others.

The most used tool is wscat, this is a great tool but it is not scriptable. I found myself pasting the same commands like login and subscribe to database, crypto and other apis over and over again.

I was after scripting wscat since a while until @gdfbacchus asked in telegram for the same thing.

Ptython is the choice as it is probably the most used language in the bitshares community.

https://pypi.python.org/pypi/websocket-client

Simply install by:

`pip install websocket-client`

Confirm the install by using the wsdump.py tool. This should do the same as wscat:

```
root@NC-PH-1346-07:~# wsdump.py ws:/localhost:8090
Press Ctrl+C to quit
> {"method": "call", "params": [1, "login", ["", ""]], "id": 2}

< {"id":2,"result":true}
> {"method": "call", "params": [1, "database", []], "id": 3}

< {"id":3,"result":2}

> {"method": "call", "params": [2, "get_objects", [["1.11.8799012"]]], "id": 4}
< {"id":4,"result":[{"id":"1.11.8799012","op":[14,{"fee":{"amount":281946,"asset_id":"1.3.0"},"issuer":"1.2.89940","asset_to_issue":{"amount":100,"asset_id":"1.3`
`.1276"},"issue_to_account":"1.2.142352","memo":{"from":"BTS8LWkZLmsnWjgtT1PNHT5XGAu1z1ueQkBHBQTVfECFVQfD3s7CF","to":"BTS6F1ZetzyG5FvjRiPjSkAjJfCqfr8AGbnGfH9FAGWZ`
M3SGVumj5","nonce":"380763353028914","message":"912991d1bb5bccccbd41dbad533836e667e5c5e9a31290c857ed6c5ea01756dd4d5893f1644c16c019170a4d0de346a2"},"extensions":[
]}],"result":[0,{}],"block_num":14086551,"trx_in_block":0,"op_in_trx":0,"virtual_op":48819}]}
>
```


Create a python script that will execute the commands one after the other and get the output:

```
from websocket import create_connection
ws = create_connection("ws://localhost:8090")
ws.send('{"method": "call", "params": [1, "login", ["", ""]], "id": 2}')
result =  ws.recv()
print result


ws.send('{"method": "call", "params": [1, "database", []], "id": 3}')
result =  ws.recv()
print result

ws.send('{"method": "call", "params": [2, "get_objects", [["1.11.8799012"]]], "id": 4}')
result =  ws.recv()
print result

ws.close()
```
Execute as:

```
root@NC-PH-1346-07:~# python testws.py 
{"id":2,"result":true}
{"id":3,"result":2}
{"id":4,"result":[{"id":"1.11.8799012","op":[14,{"fee":{"amount":281946,"asset_id":"1.3.0"},"issuer":"1.2.89940","asset_to_issue":{"amount":100,"asset_id":"1.3.1
`276"},"issue_to_account":"1.2.142352","memo":{"from":"BTS8LWkZLmsnWjgtT1PNHT5XGAu1z1ueQkBHBQTVfECFVQfD3s7CF","to":"BTS6F1ZetzyG5FvjRiPjSkAjJfCqfr8AGbnGfH9FAGWZM3
SGVumj5","nonce":"380763353028914","message":"912991d1bb5bccccbd41dbad533836e667e5c5e9a31290c857ed6c5ea01756dd4d5893f1644c16c019170a4d0de346a2"},"extensions":[]}
],"result":[0,{}],"block_num":14086551,"trx_in_block":0,"op_in_trx":0,"virtual_op":48819}]}
root@NC-PH-1346-07:~#
```


You get the results of the 3 calls inside the python script. 

Check the "Long-lived connection" sample of the websocket-client documentation in order to make a script that can receive updates.


