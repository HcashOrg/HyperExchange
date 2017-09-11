When i started working in the bitshares-core one of the first problems i found after building a node was a proper way to run a node in the background.

When a node is started, it needs to sync all the chain and this takes time, when the ssh connection between my pc and my server is lost then I need to start all over again.

Also, this will help when you want to keep your node running and turn off your pc by closing the ssh connection, here is the best way to do it.

This is a very practical setup for developers and nodemasters that connect to their servers by ssh and run nodes in there.

Meet GNU Screen https://www.gnu.org/software/screen/

We will only use just a few commands for our needs.

Type screen in your terminal and you will see a msg as:

![](http://oxarbitrage.com/bs/screen1.png)

Hit enter. You are now in a new terminal.

You can go back to the original terminal by the following combination of keys.

Press "CTRL" key and while pressed press key "a". Release "CTRL" and press "d".

You will see something like:

`[detached from 11831.pts-2.alfredo]`

This will return you to the original window detaching the screen created terminal, but anything running there will keep running in the background.

In order to gain control again to the screen window use:

`screen -r`

This will work as you only have one screen going.

Now what we need to do is create a screen terminal, run the node inside it, deattach it and turn everything off if you want and go to sleep. Then came back the next day, log in to your server, run screen -r and see how the node will be still running.

So, log in to your vps and then:

`screen`

You are now in a new terminal, navigate to your witness, start the witness by something like:

`programs/witness_node/witness_node --data-dir data/my-blockprod --genesis-json genesis/my-genesis.json  --enable-stale-production -w \""1.6.0"\" \""1.6.1"\" \""1.6.2"\" \""1.6.3"\" \""1.6.4"\" --rpc-endpoint "localhost:8090"`

Node will start syncing or whatever, you then use the key combinations explained above(ctrl-a and then d) and you get back to the original terminal.

Then you leave:

`exit`

Next day after logging in:

`screen -r`

will send you to the terminal with the running node.

More cool stuff can be done with screen but these are the basics for using it with bitshares nodes.

If you have several screens going on, the screen -r will not work for you, but follow the command suggestions and you will know how to gain control of each terminal again:

```
root@alfredo:~/bitshares-munich/recurring/bitshares-core# screen -r
There are several suitable screens on:
        11873.pts-2.alfredo     (09/01/17 20:07:00)     (Detached)
        11831.pts-2.alfredo     (09/01/17 19:55:18)     (Detached)
Type "screen [-d] -r [pid.]tty.host" to resume one of them.
root@alfredo:~/bitshares-munich/recurring/bitshares-core# 
```

In this case i will do:

`screen -r 11873.pts-2.alfredo`

to get access to one of the terminals.

To end a screen session just gain control over it and type `exit`.

One last useful command i use when the connection drops while you are inside a screen session. When you came back, the screen command will tell you that the terminal is attached but you have no control over it.

What you need to use in that case is something like:

`screen -D -r '1234.somescreensession'`

to recover your screen session.


