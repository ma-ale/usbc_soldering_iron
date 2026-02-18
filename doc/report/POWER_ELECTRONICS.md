# Circuiteria di Potenza

La parte principale del saldatore è la parte di potenza.

## USB PD
Lo standard USB PD e EPR permette potenze di uscita fino a 280W, il problema è che l'aumento di potenza viene ottenuto solamente con un aumento di tensione, mantenendo la corrente massima a 5A:
![usb_pd_iv.png](media/usb_pd_iv.png)
Quindi per mantenere la potenza di ingresso al di sotto del rating dell'alimentatore è necessario mantenere la corrente al di sotto dei 5A, superare i 5A (o poco più) manderebbe l'alimentatore in protezione spengendolo.
La maggior parte dei saldatori commerciali USB non hanno nessun tipo di filtraggio, invece usano diversi trucchi:

1. Punte non standard a 5Ohm, come il [Sequre S99](https://sequremall.com/products/sequer-s99-soldering-iron-support-pd-qc-dc-pps-power-supply-compatible-with-c245-tip-for-drone-rc-model-welding-repair-tool-anti-static-welding-pen?variant=42863253356732).
2. Switching molto veloce.
3. Limitano la tensione di ingresso per rimanere sotto al limite di corrente, limitando anche la potenza.
4. Se ne fregano il cazzo e sperano che l'alimentatore se ne freghi anch'esso.

## Altri Saldatori
È comodo confrontare altri saldatori USB-C e vedere come funzionano.

### Pinecil V2
Link alla [wiki](https://wiki.pine64.org/wiki/Pinecil), [schema elettrico](https://files.pine64.org/doc/Pinecil/Pinecil_schematic_v2.0_20220608.pdf).

### Alientek T80P
[Teardown](https://www.reddit.com/r/soldering/comments/1cm03vg/jbc_style_usb_soldering_iron_roundup_teardown/ ), [issue di IronOS](https://github.com/Ralim/IronOS/issues/1945) con foto e lista componenti.

### Miniware TS21
[Teardown](https://www.reddit.com/r/soldering/comments/1kho3lc/miniware_ts21_disassemble/), [issue di IronOS](https://github.com/Ralim/IronOS/issues/2122) con componenti.

### Sequre S60P
[Teardown](https://community.element14.com/technologies/test-and-measurement/b/blog/posts/usb-c-soldering-iron-quick-review-sequre-s60) e dettagli dei componenti, [discusione su IronOS](https://github.com/Ralim/IronOS/discussions/1806).