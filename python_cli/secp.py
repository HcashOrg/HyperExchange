import pybitcointools


pub = pybitcointools.decode_pubkey("03d299230be2bf10cad8f0b6f25c905c67e6a5896956c59adc8774c7e601946acf")
print hex(pub[1])
pybitcointools.encode_pubkey(pub,"bin_compressed")
pub1 = pybitcointools.decode_pubkey("02a4e436bede8e4ed5f6193d64619b7dddbd42a3c26dcc057de79e8f0911342647")
print hex(pub1[1])
pybitcointools.encode_pubkey(pub1,"bin_compressed")


pub = pybitcointools.decode_pubkey("0335644013e62b3576612253e1a9860e2b0c71c9033a711522606832bbcc733a44")
print hex(pub[1])
pybitcointools.encode_pubkey(pub,"bin_compressed")