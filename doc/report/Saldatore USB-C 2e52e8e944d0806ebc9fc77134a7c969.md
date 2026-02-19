# Saldatore USB-C

- https://www.printables.com/model/1127227-gridfinity-alientek-t80t80p-tool-holder-5x1 modello di un holder gridfinity per il t80p, possibile usarlo per confrontare le dimensioni delle punte
- https://www.printables.com/model/1548831-miniware-ts21/files modello a dimensione vera di un ts21, utile per confrontare le dimensioni
- https://www.printables.com/model/1435424-sliding-box-for-miniware-ts21 case per ts21 con storage per punte


# Buck

- [https://www.monolithicpower.com/en/inductor-selector-tool](https://www.monolithicpower.com/en/inductor-selector-tool)
- [Selecting Inductors for Buck Converters](https://www.ti.com/lit/an/snva038b/snva038b.pdf?ts=1769296550256)
- https://it.wikipedia.org/wiki/Convertitore_buck

![i.sstatic.net](https://i.sstatic.net/zsEMf.png)

## Versione con Buck Discreto

Una soluzione sarebbe di costruire un buck sincrono con componenti discreti

[Basic Calculation of a Buck Converter's Power Stage](https://www.ti.com/lit/an/slva477b/slva477b.pdf?ts=1769211977943)

![image.png](Saldatore%20USB-C/image%203.png)

### MOSFET

Meglio usare un doppio mosfet integrato in configurazione half-bridge. Esistono tipi con due N-MOS e tipi con P-MOS e N-MOS. La versione con due N-MOS è la migliore in termini di efficienza ma richiede necessariamente un driver particolare con circuito di bootstrap.

Alcuni MOSFET:

- https://www.lcsc.com/product-detail/C155503.html
- https://www.lcsc.com/product-detail/C3289384.html
- https://www.lcsc.com/product-detail/C485677.html
- https://www.lcsc.com/product-detail/C464808.html
- https://www.lcsc.com/product-detail/C269268.html

Potenza persa sul mosfet:

![image.png](Saldatore%20USB-C/image%204.png)

Qg=5.6nC, I=2A, Io=7.2A, Rdson=10m, vin=28, vout=18, fsw=1MHz (mosfet vishay)

Pupper=0.535W, Plower=0.185W

Ptot=0.72W

### Driver

Esistono diversi driver, per questo caso servono driver o half-bridge oppure apposta per buck sincroni, il primo richiede due segnali PWM per controllare ognuno dei mosfet separatamente, mentre il secondo accetta un solo segnale PWM.

Una specifica importante è la corrente di uscita che determina il tempo di switching dei mosfet. Vedi [questa presentazione](https://www.mouser.com/pdfdocs/Selecting-Gate-Driver-Tutorial.pdf) per capire di cosa si parla, ma deriva dalla velocità con cui si riempie la capacità di gate. 

Alcuni Driver:

- https://www.lcsc.com/product-detail/C2676972.html https://www.lcsc.com/product-detail/C460680.html
- [IR3537 / CHL8510](https://www.lcsc.com/datasheet/C537661.pdf)
- https://www.lcsc.com/product-detail/C603956.html?s_z=n_NCV51511

## Versione con Buck Integrato

Si può usare un buck integrato (switch e controllore) e regolare la tensione di feedback con il microcontrollore, ottenendo la tensione che volgio io.

https://www.youtube.com/watch?v=hyLYPu-4EN8

Non è una cosa troppo comune ne tanto facile da fare, alcuni dicono che con l’ADC del micro non si va da nessuna parte, altri dicono di si ma di usare un ADC in corrente. Alcuni riferimenti:

- https://forum.allaboutcircuits.com/threads/digitally-controlled-buck-converter.75554/
- https://www.ednasia.com/use-a-current-mirror-to-control-a-power-supply/
- https://electronics.stackexchange.com/questions/700494/mcu-controlled-buck-converter-ic

Uno dei problemi di questo approccio è che i buck commerciali, anche di alta potenza sono fatti per essere “buoni”, poco ripple e tensione molto precisa. Mentre qello che serve a me è di rimanere sotto la potenza nominale impostata, quindi è molto overkill. Inoltre molti dei buck di alta potenza usano molti componenti di contorno aumentando ancora il prezzo.

Ricerca su Gemini: https://gemini.google.com/app/65933a590936e3ec

Lista di buck fighi:

- [SiC461](https://www.lcsc.com/datasheet/C499531.pdf) la Vishay ha addirittura un calcolatore fatto apposta: https://vishay.transim.com/landing
- [~~SY8308~~](https://www.lcsc.com/datasheet/C7427429.pdf) https://www.lcsc.com/product-detail/C125897.html, https://www.lcsc.com/product-detail/C207642.html
- [AOZ2261AQI](https://www.lcsc.com/datasheet/C2071876.pdf)
- [XR76203](https://www.lcsc.com/datasheet/C555356.pdf) da usare il [xr76208](https://www.lcsc.com/product-detail/C555356.html?s_z=n_xr76208)
- https://www.lcsc.com/product-detail/C3188410.html [Datasheet in inglese](https://www.ti.com/lit/ds/symlink/lm61495-q1.pdf?ts=1770121316572)
- [NDP13802QB](https://www.lcsc.com/datasheet/C7420577.pdf) oppure https://www.lcsc.com/product-detail/C7420585.html?s_z=n_ndp1480
- [TPS56837Hx](https://www.ti.com/lit/ds/symlink/tps56837h.pdf?ts=1769345326940) …HA non ha il discharge ma costa di più

Saldatori che usano questo metodo: 

- Solder ninja pen, [hardware](https://github.com/sitronlabs/Solder-Ninja-Pen-Electronics), [software](https://github.com/sitronlabs/Solder-Ninja-Pen-Firmware), [ironos](https://github.com/Ralim/IronOS/issues/1989). Soli 45W ma usa un buck integrato [TPS621351](https://www.lcsc.com/product-detail/C2070706.html?s_z=n_TPS621351) quindi il software di controllo è quello. Usa questo DAC ([DAC5311](https://www.lcsc.com/product-detail/C191582.html?s_z=n_DAC5311)) per controllare il feedback del buck.
- [iFixit Hub](https://www.ifixit.com/Device/iFixit_Soldering) le [schematic](https://www.ifixit.com/Document/WOhhObf5OxsyA4Xh/ES-SI-2100-R15.pdf?referrer=wiki%3A570611) sono pubbliche, 100W ma dalle recensioni non sembra arrivarci davvero. Usa un [MP2329](https://www.monolithicpower.com/en/mp2329.html) buck fino a 20V di Vin e il dac integrato dell’STM32 per controllare il feedback. Le dimensioni del controllore sono piccole, forse per questo non riesce ad arrivare ai 100W senza andare in protezione.

## Versione con Half-Bridge e Micro

La differenza di questo approccio è l’uso di un half-bridge che però ha anche integrato il driver per i mosfet. In questo modo l’interità del controllo deve essere fatta dal microcontrollore però si risparmia sulle feature inutili e tutti i componenti di contorno che ne vengono.

Alcuni MOSFET:

- https://www.lcsc.com/product-detail/C3654920.html?s_z=n_SiC641

Ricerca su Gemini: https://gemini.google.com/app/7377e4ba09fedab9

Calcolo dell’efficienza:

- https://fscdn.rohm.com/en/products/databook/applinote/ic/power/switching_regulator/buck_converter_efficiency_app-e.pdf
- https://www.ti.com/lit/an/slvaeq9/slvaeq9.pdf?ts=1769284164843&ref_url=https%253A%252F%252Fwww.google.com%252F
- https://www.hajim.rochester.edu/ece/sites/friedman/papers/ICECS_10.pdf

## Buck Multifase

Per ridurre il picco di corrente dall’ingresso bisogna necessariamente aggiungere più buck in parallelo. 

https://www.ti.com/lit/an/slva882b/slva882b.pdf

Il picco di corrente di un buck è pari alla corrente media di uscita più la metà del ripple di corrente, che per design è maggiore al 40%. Quindi a livello di corrente di picco non si è migliorato nulla.

Nel buck multifase il picco di corrente è diviso per il numero di fasi, quindi per me ne bastano due.

L’implementazione di un buck multifase, per essere abbastanza compatta, deve essere fatta o con power stage integrati controllati dal microcontrollore, o in modo discreto con mos e driver. Ho scelto mos e driver 2x2mm.

alcuni n-mos doppi:

- [UT6K3](https://www.lcsc.com/datasheet/C5336512.pdf)
- [AON7934](https://www.lcsc.com/datasheet/C485677.pdf)
- [IRLHS6376TRPBF](https://www.lcsc.com/datasheet/C538153.pdf)
- [SiA918EDJ](https://www.lcsc.com/datasheet/C727447.pdf)

Alcuni gate driver:

- [BDR2L00](https://www.lcsc.com/datasheet/C5371998.pdf)
- [NCP81161](https://www.lcsc.com/datasheet/C603811.pdf) [NCP81151](https://www.lcsc.com/datasheet/C603810.pdf)
- https://www.lcsc.com/product-detail/C41414784.html?s_z=s_Integrated%2520Circuits%2520%28ICs%29%257CPower%2520Management%2520%28PMIC%29%257CGate%2520Drivers

# Filtro Pi per Ridurre i Picchi di Corrente

Non serve allora che si usi nessun tipo di controllore, basta che la frequenza di PWM della parte switching sia maggiore della frequenza di taglio del filtro.

Si risparmia in termini di spazio perchè basta un mosfet e il suo driver, allora posso rendere l’induttore più grande e usare pochi condensatori. Inoltre in termini di controllo è molto molto più sempplice, si risparmia su DAC, resistenze di precisione e altri componenti particolari. Essendo un solo NMOS allora anche la dissiapzione è minore e le perdite switching possono essere piccole rispetto al buck dato che la frequenza di switching rimane sulle decine di kHz.

![image.png](Saldatore%20USB-C/image%206.png)

### Induttori


Un problema è la corrente di inrush che spengerebbe l’alimentatore, quindi bisogna attivamente limitare la corrente ed effettuare un soft-start. Esistono vari integrati fatti apposta.

Integrati per protezione di inrush (efuse, hotswap controllers, etc.):

- [MX26631DL](https://www.lcsc.com/datasheet/C18197523.pdf)
- https://www.ti.com/product/TPS2663
- [DPS1135](https://www.diodes.com/datasheet/download/DPS1135.pdf) https://www.lcsc.com/product-detail/C2150216.html?s_z=n_DPS1135
- https://www.maxinmicro.com/products/dzbxs/wzgl
- https://www.lcsc.com/product-detail/C47967144.html?s_z=n_LMX5069DL
- [RT1720](https://www.lcsc.com/datasheet/C147997.pdf)
- [TPS249x](https://www.lcsc.com/datasheet/C273636.pdf)
- [TPS25980](https://www.ti.com/lit/ds/symlink/tps25980.pdf?ts=1770547961735)

### High-Side Gate Drivers

| **LINK** | **NOTE** | **PACKAGE** | **DIODO BOOT** |
| --- | --- | --- | --- |
| https://www.lcsc.com/product-detail/C5795658.html | half bridge, consigliato per notebook e quindi tensione minore | dfn 2x2 |  |
| https://www.lcsc.com/product-detail/C116731.html | half bridge per buck | dfn 2x2 |  |
| https://www.lcsc.com/product-detail/C892859.html | half bridge per buck | dfn 2x2 |  |
| https://www.lcsc.com/product-detail/C606336.html | half bridge per buck | dfn 3x3 |  |
| https://www.lcsc.com/product-detail/C533032.html | low-side ma anche high-side con configurazione strana, input differenziale | sot 23 |  |
| https://www.lcsc.com/product-detail/C5481755.html | high e low-side per mosfet SiC, input differenziale e configurazione strana | sot 23 |  |
| https://www.lcsc.com/product-detail/C964602.html?s_z=n_ISL95808 | half bridge per buck | dfn 2x2 | interno |
| https://www.lcsc.com/product-detail/C2677203.html?s_z=n_MAX15054 | logica 5V | sot 23 | esterno |
| https://www.lcsc.com/product-detail/C126923.html?s_z=n_IRS10752L |  | sot 23 | esteno |
| https://www.lcsc.com/product-detail/C495818.html |  | sot 23 | esterno |
| https://www.lcsc.com/product-detail/C538346.html?utm_source=octopart&utm_medium=cpc&utm_campaign=IRS25752LTRPBF |  | sot 23 | esterno |
| https://www.lcsc.com/product-detail/C41414522.html | pin-compatible con quelli infineon | sot 23 | esterno |
| https://www.lcsc.com/product-detail/C603811.html | half-bridge per buck, discontinuato | dfn 2x2 | interno |
| https://www.lcsc.com/product-detail/C603810.html | half bridge per buck, versione di replacement per NCP81161 | dfn 2x2 | interno |
| https://www.lcsc.com/product-detail/C2677131.html | half bridge per buck | dfn 3x3 | interno |
| https://www.lcsc.com/product-detail/C41414473.html | half bridge per buck | dfn 3x3 | esterno |
| https://www.lcsc.com/product-detail/C154581.html | half bridge per buck | dfn 3x3 | interno |

**Switch per Input Side:**

- https://www.lcsc.com/product-detail/C5219206.html?s_z=n_LM74502

### Condensatori

https://article.murata.com/en-eu/article/temperature-characteristics-electrostatic-capacitance

| https://www.lcsc.com/product-detail/C47117456.html |  |  |
| --- | --- | --- |
| https://www.lcsc.com/product-detail/C338080.html |  |  |
| https://www.lcsc.com/product-detail/C6119901.html |  |  |
| https://www.lcsc.com/product-detail/C22367832.html |  |  |

# Altri Componenti

### Bottoni

| https://www.lcsc.com/product-detail/C963235.html |  |  |
| --- | --- | --- |
| https://www.lcsc.com/product-detail/C2906288.html |  |  |
| https://www.lcsc.com/product-detail/C778158.html |  |  |
| https://www.lcsc.com/product-detail/C49234146.html |  |  |
| https://www.lcsc.com/product-detail/C393942.html |  |  |
| https://www.lcsc.com/product-detail/C79167.html |  |  |
| https://www.lcsc.com/product-detail/C2906288.html |  |  |

### Induttori per regolatore 3.3V

| **MODELLO** | **PACKAGE** | **AMP RATING** | **DCR** |
| --- | --- | --- | --- |
| https://www.lcsc.com/product-detail/C486332.html | 0805 | 550mA | 270mΩ |
| https://www.lcsc.com/product-detail/C48689504.html | 1008 | 1.4A | 270mΩ |
| https://www.lcsc.com/product-detail/C52741389.html | 1008 | 1A | 380mΩ |
| https://www.lcsc.com/product-detail/C52741377.html | 0806 | 820mA | 545mΩ |
| https://www.lcsc.com/product-detail/C5289257.html | 1008 | 520mA | 810mΩ |
| https://www.lcsc.com/product-detail/C52741450.html | 1008 | 1.1A | 320mΩ |
| https://www.lcsc.com/product-detail/C53198536.html | 1008 | 1.6A | 245mΩ |
| https://www.lcsc.com/product-detail/C216121.html | 0806 | 700mA | 500mΩ |
| https://www.lcsc.com/product-detail/C5451617.html | 1008 | 1.05A | 360mΩ |
| https://www.lcsc.com/product-detail/C602017.html | 1008 | 1.1A | 305mΩ |
| https://www.lcsc.com/product-detail/C601999.html | 0806 | 820mA | 500mΩ |
| https://www.lcsc.com/product-detail/C5832375.html | 1008 | 1.6A | 245mΩ |
| https://www.lcsc.com/product-detail/C5832361.html | 1008 | 1.6A | 270mΩ |

### Encoder

| **MODELLO** | **SWITCH** | **DIMENSIONE** |
| --- | --- | --- |
| https://www.lcsc.com/product-detail/C17702124.html | si | 4.8x3.9x3.5 |
| https://www.lcsc.com/product-detail/C49302686.html | no | 4.8x3.9x2.9 |
| https://www.lcsc.com/product-detail/C52995052.html | no | 4.8x3.9x2.9 |
| https://www.lcsc.com/product-detail/C52995053.html | no | 4.8x3.9x2.9 |
| https://www.lcsc.com/product-detail/C41430892.html https://www.lcsc.com/product-detail/C46818248.html https://www.lcsc.com/product-detail/C2925423.html | si |  |
| https://www.lcsc.com/product-detail/C125039.html |  |  |

Doveviene usato l’encoder che mi piace:

- https://www.instructables.com/Arduino-Walkie-Talkie/
- https://www.reddit.com/r/embedded/comments/1qjdnbd/early_build_of_ui_for_midi_pedal_adapter_im_making/?tl=it&sort=new
- https://flatfootfox.com/the-typeboy-mk-ii/
- https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/Jumperless23V50/Footprints%20and%20Symbols/JumprlessFootprints.pretty