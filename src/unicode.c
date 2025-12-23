/* Gemini Browser - Unicode fallback for webOS (Unicode 6.0) */
#include "unicode.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Fallback entry: codepoint -> replacement string */
typedef struct {
    uint32_t codepoint;
    const char *replacement;
} FallbackEntry;

/*
 * Fallback table for characters not in Unicode 6.0
 * Sorted by codepoint for binary search
 */
static const FallbackEntry fallbacks[] = {
    /* Fancy quotes and punctuation */
    { 0x2018, "'" },    /* LEFT SINGLE QUOTATION MARK */
    { 0x2019, "'" },    /* RIGHT SINGLE QUOTATION MARK */
    { 0x201C, "\"" },   /* LEFT DOUBLE QUOTATION MARK */
    { 0x201D, "\"" },   /* RIGHT DOUBLE QUOTATION MARK */
    { 0x2026, "..." },  /* HORIZONTAL ELLIPSIS */
    { 0x2013, "-" },    /* EN DASH */
    { 0x2014, "--" },   /* EM DASH */

    /* Arrows (some newer ones) */
    { 0x2B05, "<-" },   /* LEFTWARDS BLACK ARROW */
    { 0x2B06, "^" },    /* UPWARDS BLACK ARROW */
    { 0x2B07, "v" },    /* DOWNWARDS BLACK ARROW */
    { 0x2B08, "->" },   /* NORTH EAST BLACK ARROW */
    { 0x2B95, "->" },   /* RIGHTWARDS BLACK ARROW */
    { 0x27A1, "->" },   /* BLACK RIGHTWARDS ARROW */

    /* Common emoji - Basic smileys */
    { 0x1F600, ":)" },  /* GRINNING FACE */
    { 0x1F601, ":D" },  /* GRINNING FACE WITH SMILING EYES */
    { 0x1F602, "XD" },  /* FACE WITH TEARS OF JOY */
    { 0x1F603, ":)" },  /* SMILING FACE WITH OPEN MOUTH */
    { 0x1F604, ":)" },  /* SMILING FACE WITH OPEN MOUTH AND SMILING EYES */
    { 0x1F605, ":)" },  /* SMILING FACE WITH OPEN MOUTH AND COLD SWEAT */
    { 0x1F606, "XD" },  /* SMILING FACE WITH OPEN MOUTH AND TIGHTLY-CLOSED EYES */
    { 0x1F607, ":)" },  /* SMILING FACE WITH HALO */
    { 0x1F608, ">:)" }, /* SMILING FACE WITH HORNS */
    { 0x1F609, ";)" },  /* WINKING FACE */
    { 0x1F60A, ":)" },  /* SMILING FACE WITH SMILING EYES */
    { 0x1F60B, ":P" },  /* FACE SAVOURING DELICIOUS FOOD */
    { 0x1F60C, ":)" },  /* RELIEVED FACE */
    { 0x1F60D, "<3" },  /* SMILING FACE WITH HEART-EYES */
    { 0x1F60E, "B)" },  /* SMILING FACE WITH SUNGLASSES */
    { 0x1F60F, ":/" },  /* SMIRKING FACE */
    { 0x1F610, ":|" },  /* NEUTRAL FACE */
    { 0x1F611, ":|" },  /* EXPRESSIONLESS FACE */
    { 0x1F612, ":/" },  /* UNAMUSED FACE */
    { 0x1F613, ":S" },  /* FACE WITH COLD SWEAT */
    { 0x1F614, ":(" },  /* PENSIVE FACE */
    { 0x1F615, ":/" },  /* CONFUSED FACE */
    { 0x1F616, ":S" },  /* CONFOUNDED FACE */
    { 0x1F617, ":*" },  /* KISSING FACE */
    { 0x1F618, ":*" },  /* FACE THROWING A KISS */
    { 0x1F619, ":*" },  /* KISSING FACE WITH SMILING EYES */
    { 0x1F61A, ":*" },  /* KISSING FACE WITH CLOSED EYES */
    { 0x1F61B, ":P" },  /* FACE WITH STUCK-OUT TONGUE */
    { 0x1F61C, ";P" },  /* FACE WITH STUCK-OUT TONGUE AND WINKING EYE */
    { 0x1F61D, "XP" },  /* FACE WITH STUCK-OUT TONGUE AND TIGHTLY-CLOSED EYES */
    { 0x1F61E, ":(" },  /* DISAPPOINTED FACE */
    { 0x1F61F, ":(" },  /* WORRIED FACE */
    { 0x1F620, ">:(" }, /* ANGRY FACE */
    { 0x1F621, ">:(" }, /* POUTING FACE */
    { 0x1F622, ":'(" }, /* CRYING FACE */
    { 0x1F623, ">_<" }, /* PERSEVERING FACE */
    { 0x1F624, ">:(" }, /* FACE WITH LOOK OF TRIUMPH */
    { 0x1F625, ":'(" }, /* DISAPPOINTED BUT RELIEVED FACE */
    { 0x1F626, ":(" },  /* FROWNING FACE WITH OPEN MOUTH */
    { 0x1F627, ":(" },  /* ANGUISHED FACE */
    { 0x1F628, ":O" },  /* FEARFUL FACE */
    { 0x1F629, ":(" },  /* WEARY FACE */
    { 0x1F62A, ":(" },  /* SLEEPY FACE */
    { 0x1F62B, ":(" },  /* TIRED FACE */
    { 0x1F62C, ">:(" }, /* GRIMACING FACE */
    { 0x1F62D, ":'(" }, /* LOUDLY CRYING FACE */
    { 0x1F62E, ":O" },  /* FACE WITH OPEN MOUTH */
    { 0x1F62F, ":O" },  /* HUSHED FACE */
    { 0x1F630, ":O" },  /* FACE WITH OPEN MOUTH AND COLD SWEAT */
    { 0x1F631, ":O" },  /* FACE SCREAMING IN FEAR */
    { 0x1F632, ":O" },  /* ASTONISHED FACE */
    { 0x1F633, ":$" },  /* FLUSHED FACE */
    { 0x1F634, "zzZ" }, /* SLEEPING FACE */
    { 0x1F635, "x_x" }, /* DIZZY FACE */
    { 0x1F636, ":|" },  /* FACE WITHOUT MOUTH */
    { 0x1F637, ":)" },  /* FACE WITH MEDICAL MASK */
    { 0x1F638, ":)" },  /* GRINNING CAT FACE WITH SMILING EYES */
    { 0x1F639, "XD" },  /* CAT FACE WITH TEARS OF JOY */
    { 0x1F63A, ":)" },  /* SMILING CAT FACE WITH OPEN MOUTH */
    { 0x1F63B, "<3" },  /* SMILING CAT FACE WITH HEART-EYES */
    { 0x1F63C, ":)" },  /* CAT FACE WITH WRY SMILE */
    { 0x1F63D, ":*" },  /* KISSING CAT FACE WITH CLOSED EYES */
    { 0x1F63E, ">:(" }, /* POUTING CAT FACE */
    { 0x1F63F, ":'(" }, /* CRYING CAT FACE */
    { 0x1F640, ":O" },  /* WEARY CAT FACE */

    /* Hearts and symbols */
    { 0x2764, "<3" },   /* HEAVY BLACK HEART */
    { 0x2665, "<3" },   /* BLACK HEART SUIT */
    { 0x1F494, "</3" }, /* BROKEN HEART */
    { 0x1F495, "<3<3" },/* TWO HEARTS */
    { 0x1F496, "<3" },  /* SPARKLING HEART */
    { 0x1F497, "<3" },  /* GROWING HEART */
    { 0x1F498, "<3" },  /* HEART WITH ARROW */
    { 0x1F499, "<3" },  /* BLUE HEART */
    { 0x1F49A, "<3" },  /* GREEN HEART */
    { 0x1F49B, "<3" },  /* YELLOW HEART */
    { 0x1F49C, "<3" },  /* PURPLE HEART */
    { 0x1F49D, "<3" },  /* HEART WITH RIBBON */
    { 0x1F49E, "<3" },  /* REVOLVING HEARTS */
    { 0x1F49F, "<3" },  /* HEART DECORATION */

    /* Common objects */
    { 0x1F4A1, "[idea]" },    /* LIGHT BULB */
    { 0x1F4A4, "zzZ" },       /* SLEEPING SYMBOL */
    { 0x1F4A5, "[boom]" },    /* COLLISION SYMBOL */
    { 0x1F4A9, "[poop]" },    /* PILE OF POO */
    { 0x1F4AA, "[flex]" },    /* FLEXED BICEPS */
    { 0x1F4AF, "[100]" },     /* HUNDRED POINTS SYMBOL */
    { 0x1F4BB, "[PC]" },      /* PERSONAL COMPUTER */
    { 0x1F4BE, "[disk]" },    /* FLOPPY DISK */
    { 0x1F4BF, "[CD]" },      /* OPTICAL DISC */
    { 0x1F4C1, "[folder]" },  /* FILE FOLDER */
    { 0x1F4C2, "[folder]" },  /* OPEN FILE FOLDER */
    { 0x1F4C4, "[doc]" },     /* PAGE FACING UP */
    { 0x1F4D6, "[book]" },    /* OPEN BOOK */
    { 0x1F4D7, "[book]" },    /* GREEN BOOK */
    { 0x1F4D8, "[book]" },    /* BLUE BOOK */
    { 0x1F4D9, "[book]" },    /* ORANGE BOOK */
    { 0x1F4DA, "[books]" },   /* BOOKS */
    { 0x1F4DD, "[memo]" },    /* MEMO */
    { 0x1F4E7, "[email]" },   /* E-MAIL SYMBOL */
    { 0x1F4F1, "[phone]" },   /* MOBILE PHONE */
    { 0x1F4F7, "[camera]" },  /* CAMERA */
    { 0x1F4F9, "[video]" },   /* VIDEO CAMERA */
    { 0x1F50B, "[battery]" }, /* BATTERY */
    { 0x1F50C, "[plug]" },    /* ELECTRIC PLUG */
    { 0x1F50D, "[search]" },  /* LEFT-POINTING MAGNIFYING GLASS */
    { 0x1F50E, "[search]" },  /* RIGHT-POINTING MAGNIFYING GLASS */
    { 0x1F512, "[lock]" },    /* LOCK */
    { 0x1F513, "[unlock]" },  /* OPEN LOCK */
    { 0x1F514, "[bell]" },    /* BELL */
    { 0x1F516, "[bookmark]" },/* BOOKMARK */
    { 0x1F517, "[link]" },    /* LINK SYMBOL */

    /* Hands */
    { 0x1F44D, "[+1]" },      /* THUMBS UP SIGN */
    { 0x1F44E, "[-1]" },      /* THUMBS DOWN SIGN */
    { 0x1F44B, "[wave]" },    /* WAVING HAND SIGN */
    { 0x1F44C, "[OK]" },      /* OK HAND SIGN */
    { 0x1F44F, "[clap]" },    /* CLAPPING HANDS SIGN */
    { 0x1F450, "[hands]" },   /* OPEN HANDS SIGN */
    { 0x1F64F, "[pray]" },    /* PERSON WITH FOLDED HANDS */

    /* People */
    { 0x1F464, "[person]" },  /* BUST IN SILHOUETTE */
    { 0x1F465, "[people]" },  /* BUSTS IN SILHOUETTE */
    { 0x1F466, "[boy]" },     /* BOY */
    { 0x1F467, "[girl]" },    /* GIRL */
    { 0x1F468, "[man]" },     /* MAN */
    { 0x1F469, "[woman]" },   /* WOMAN */
    { 0x1F46A, "[family]" },  /* FAMILY */
    { 0x1F46B, "[couple]" },  /* MAN AND WOMAN HOLDING HANDS */
    { 0x1F46C, "[men]" },     /* TWO MEN HOLDING HANDS */
    { 0x1F46D, "[women]" },   /* TWO WOMEN HOLDING HANDS */
    { 0x1F46E, "[police]" },  /* POLICE OFFICER */
    { 0x1F46F, "[dancers]" }, /* WOMAN WITH BUNNY EARS */
    { 0x1F470, "[bride]" },   /* BRIDE WITH VEIL */
    { 0x1F471, "[person]" },  /* PERSON WITH BLOND HAIR */
    { 0x1F472, "[person]" },  /* MAN WITH GUA PI MAO */
    { 0x1F473, "[person]" },  /* MAN WITH TURBAN */
    { 0x1F474, "[elder]" },   /* OLDER MAN */
    { 0x1F475, "[elder]" },   /* OLDER WOMAN */
    { 0x1F476, "[baby]" },    /* BABY */
    { 0x1F477, "[worker]" },  /* CONSTRUCTION WORKER */
    { 0x1F478, "[princess]" },/* PRINCESS */
    { 0x1F479, "[ogre]" },    /* JAPANESE OGRE */
    { 0x1F47A, "[goblin]" },  /* JAPANESE GOBLIN */
    { 0x1F47B, "[ghost]" },   /* GHOST */
    { 0x1F47C, "[angel]" },   /* BABY ANGEL */
    { 0x1F47D, "[alien]" },   /* EXTRATERRESTRIAL ALIEN */
    { 0x1F47E, "[alien]" },   /* ALIEN MONSTER */
    { 0x1F47F, "[imp]" },     /* IMP */
    { 0x1F480, "[skull]" },   /* SKULL */
    { 0x1F481, "[person]" },  /* INFORMATION DESK PERSON */
    { 0x1F482, "[guard]" },   /* GUARDSMAN */
    { 0x1F483, "[dancer]" },  /* DANCER */
    { 0x1F484, "[lipstick]" },/* LIPSTICK */
    { 0x1F485, "[nails]" },   /* NAIL POLISH */
    { 0x1F486, "[massage]" }, /* FACE MASSAGE */
    { 0x1F487, "[haircut]" }, /* HAIRCUT */
    { 0x1F48B, "[kiss]" },    /* KISS MARK */
    { 0x1F48C, "[love letter]" }, /* LOVE LETTER */
    { 0x1F48D, "[ring]" },    /* RING */
    { 0x1F48E, "[gem]" },     /* GEM STONE */
    { 0x1F48F, "[kiss]" },    /* KISS */
    { 0x1F490, "[bouquet]" }, /* BOUQUET */
    { 0x1F491, "[couple]" },  /* COUPLE WITH HEART */
    { 0x1F492, "[wedding]" }, /* WEDDING */
    { 0x1F493, "<3" },        /* BEATING HEART */

    /* Weather/nature/globe */
    { 0x2600, "[sun]" },      /* BLACK SUN WITH RAYS */
    { 0x2601, "[cloud]" },    /* CLOUD */
    { 0x2602, "[umbrella]" }, /* UMBRELLA */
    { 0x2603, "[snowman]" },  /* SNOWMAN */
    { 0x2604, "[comet]" },    /* COMET */
    { 0x2614, "[rain]" },     /* UMBRELLA WITH RAIN DROPS */
    { 0x26A1, "[zap]" },      /* HIGH VOLTAGE SIGN */
    { 0x1F308, "[rainbow]" }, /* RAINBOW */
    { 0x1F30D, "[globe]" },   /* EARTH GLOBE EUROPE-AFRICA */
    { 0x1F30E, "[globe]" },   /* EARTH GLOBE AMERICAS */
    { 0x1F30F, "[globe]" },   /* EARTH GLOBE ASIA-AUSTRALIA */
    { 0x1F310, "[web]" },     /* GLOBE WITH MERIDIANS */
    { 0x1F319, "[moon]" },    /* CRESCENT MOON */
    { 0x1F31F, "[star]" },    /* GLOWING STAR */
    { 0x1F320, "[star]" },    /* SHOOTING STAR */
    { 0x1F331, "[plant]" },   /* SEEDLING */
    { 0x1F332, "[tree]" },    /* EVERGREEN TREE */
    { 0x1F333, "[tree]" },    /* DECIDUOUS TREE */
    { 0x1F337, "[flower]" },  /* TULIP */
    { 0x1F338, "[flower]" },  /* CHERRY BLOSSOM */
    { 0x1F339, "[rose]" },    /* ROSE */
    { 0x1F33A, "[flower]" },  /* HIBISCUS */
    { 0x1F33B, "[flower]" },  /* SUNFLOWER */
    { 0x1F33C, "[flower]" },  /* BLOSSOM */
    { 0x1F340, "[clover]" },  /* FOUR LEAF CLOVER */
    { 0x1F525, "[fire]" },    /* FIRE */
    { 0x1F4A7, "[drop]" },    /* DROPLET */
    { 0x1F940, "[flower]" },  /* WILTED FLOWER */

    /* Animals */
    { 0x1F400, "[mouse]" },   /* RAT */
    { 0x1F401, "[mouse]" },   /* MOUSE */
    { 0x1F402, "[ox]" },      /* OX */
    { 0x1F403, "[cow]" },     /* WATER BUFFALO */
    { 0x1F404, "[cow]" },     /* COW */
    { 0x1F405, "[tiger]" },   /* TIGER */
    { 0x1F406, "[leopard]" }, /* LEOPARD */
    { 0x1F407, "[rabbit]" },  /* RABBIT */
    { 0x1F408, "[cat]" },     /* CAT */
    { 0x1F409, "[dragon]" },  /* DRAGON */
    { 0x1F40A, "[croc]" },    /* CROCODILE */
    { 0x1F40B, "[whale]" },   /* WHALE */
    { 0x1F40C, "[snail]" },   /* SNAIL */
    { 0x1F40D, "[snake]" },   /* SNAKE */
    { 0x1F40E, "[horse]" },   /* HORSE */
    { 0x1F40F, "[ram]" },     /* RAM */
    { 0x1F410, "[goat]" },    /* GOAT */
    { 0x1F411, "[sheep]" },   /* SHEEP */
    { 0x1F412, "[monkey]" },  /* MONKEY */
    { 0x1F413, "[rooster]" }, /* ROOSTER */
    { 0x1F414, "[chicken]" }, /* CHICKEN */
    { 0x1F415, "[dog]" },     /* DOG */
    { 0x1F416, "[pig]" },     /* PIG */
    { 0x1F417, "[boar]" },    /* BOAR */
    { 0x1F418, "[elephant]" },/* ELEPHANT */
    { 0x1F419, "[octopus]" }, /* OCTOPUS */
    { 0x1F41A, "[shell]" },   /* SPIRAL SHELL */
    { 0x1F41B, "[bug]" },     /* BUG */
    { 0x1F41C, "[ant]" },     /* ANT */
    { 0x1F41D, "[bee]" },     /* HONEYBEE */
    { 0x1F41E, "[ladybug]" }, /* LADY BEETLE */
    { 0x1F41F, "[fish]" },    /* FISH */
    { 0x1F420, "[fish]" },    /* TROPICAL FISH */
    { 0x1F421, "[fish]" },    /* BLOWFISH */
    { 0x1F422, "[turtle]" },  /* TURTLE */
    { 0x1F423, "[chick]" },   /* HATCHING CHICK */
    { 0x1F424, "[chick]" },   /* BABY CHICK */
    { 0x1F425, "[chick]" },   /* FRONT-FACING BABY CHICK */
    { 0x1F426, "[bird]" },    /* BIRD */
    { 0x1F427, "[penguin]" }, /* PENGUIN */
    { 0x1F428, "[koala]" },   /* KOALA */
    { 0x1F429, "[poodle]" },  /* POODLE */
    { 0x1F42A, "[camel]" },   /* DROMEDARY CAMEL */
    { 0x1F42B, "[camel]" },   /* BACTRIAN CAMEL */
    { 0x1F42C, "[dolphin]" }, /* DOLPHIN */
    { 0x1F42D, "[mouse]" },   /* MOUSE FACE */
    { 0x1F42E, "[cow]" },     /* COW FACE */
    { 0x1F42F, "[tiger]" },   /* TIGER FACE */
    { 0x1F430, "[rabbit]" },  /* RABBIT FACE */
    { 0x1F431, "[cat]" },     /* CAT FACE */
    { 0x1F432, "[dragon]" },  /* DRAGON FACE */
    { 0x1F433, "[whale]" },   /* SPOUTING WHALE */
    { 0x1F434, "[horse]" },   /* HORSE FACE */
    { 0x1F435, "[monkey]" },  /* MONKEY FACE */
    { 0x1F436, "[dog]" },     /* DOG FACE */
    { 0x1F437, "[pig]" },     /* PIG FACE */
    { 0x1F438, "[frog]" },    /* FROG FACE */
    { 0x1F439, "[hamster]" }, /* HAMSTER FACE */
    { 0x1F43A, "[wolf]" },    /* WOLF FACE */
    { 0x1F43B, "[bear]" },    /* BEAR FACE */
    { 0x1F43C, "[panda]" },   /* PANDA FACE */
    { 0x1F43D, "[pig]" },     /* PIG NOSE */
    { 0x1F43E, "[paw]" },     /* PAW PRINTS */
    { 0x1F98A, "[fox]" },     /* FOX FACE */
    { 0x1F98B, "[butterfly]" },/* BUTTERFLY */
    { 0x1F98C, "[deer]" },    /* DEER */
    { 0x1F98D, "[gorilla]" }, /* GORILLA */
    { 0x1F98E, "[lizard]" },  /* LIZARD */
    { 0x1F98F, "[rhino]" },   /* RHINOCEROS */
    { 0x1F990, "[shrimp]" },  /* SHRIMP */
    { 0x1F991, "[squid]" },   /* SQUID */
    { 0x1F992, "[giraffe]" }, /* GIRAFFE */
    { 0x1F993, "[zebra]" },   /* ZEBRA */
    { 0x1F994, "[hedgehog]" },/* HEDGEHOG */
    { 0x1F995, "[dino]" },    /* SAUROPOD */
    { 0x1F996, "[t-rex]" },   /* T-REX */
    { 0x1F997, "[cricket]" }, /* CRICKET */
    { 0x1F9A0, "[germ]" },    /* MICROBE */
    { 0x1F9A5, "[sloth]" },   /* SLOTH */
    { 0x1F9A6, "[otter]" },   /* OTTER */
    { 0x1F9A7, "[orangutan]" },/* ORANGUTAN */
    { 0x1F9A8, "[skunk]" },   /* SKUNK */
    { 0x1F9A9, "[flamingo]" },/* FLAMINGO */
    { 0x1F577, "[spider]" },  /* SPIDER */
    { 0x1F578, "[web]" },     /* SPIDER WEB */

    /* Food/drink */
    { 0x1F355, "[pizza]" },   /* SLICE OF PIZZA */
    { 0x1F354, "[burger]" },  /* HAMBURGER */
    { 0x1F37A, "[beer]" },    /* BEER MUG */
    { 0x1F37B, "[cheers]" },  /* CLINKING BEER MUGS */
    { 0x1F377, "[wine]" },    /* WINE GLASS */
    { 0x1F370, "[cake]" },    /* SHORTCAKE */
    { 0x1F382, "[bday]" },    /* BIRTHDAY CAKE */
    { 0x2615, "[coffee]" },   /* HOT BEVERAGE */

    /* Misc symbols */
    { 0x2705, "[check]" },    /* WHITE HEAVY CHECK MARK */
    { 0x2714, "[check]" },    /* HEAVY CHECK MARK */
    { 0x2716, "[X]" },        /* HEAVY MULTIPLICATION X */
    { 0x274C, "[X]" },        /* CROSS MARK */
    { 0x274E, "[X]" },        /* CROSS MARK BUTTON */
    { 0x2728, "[sparkle]" },  /* SPARKLES */
    { 0x2733, "*" },          /* EIGHT SPOKED ASTERISK */
    { 0x2734, "*" },          /* EIGHT POINTED BLACK STAR */
    { 0x2747, "*" },          /* SPARKLE */
    { 0x2757, "!" },          /* HEAVY EXCLAMATION MARK SYMBOL */
    { 0x2753, "?" },          /* BLACK QUESTION MARK ORNAMENT */
    { 0x2754, "?" },          /* WHITE QUESTION MARK ORNAMENT */
    { 0x2755, "!" },          /* WHITE EXCLAMATION MARK ORNAMENT */
    { 0x2763, "!" },          /* HEAVY HEART EXCLAMATION MARK ORNAMENT */
    { 0x2795, "+" },          /* HEAVY PLUS SIGN */
    { 0x2796, "-" },          /* HEAVY MINUS SIGN */
    { 0x2797, "/" },          /* HEAVY DIVISION SIGN */
    { 0x27B0, "~" },          /* CURLY LOOP */
    { 0x27BF, "~" },          /* DOUBLE CURLY LOOP */

    /* Zodiac (just use abbreviations) */
    { 0x2648, "[Aries]" },
    { 0x2649, "[Taurus]" },
    { 0x264A, "[Gemini]" },
    { 0x264B, "[Cancer]" },
    { 0x264C, "[Leo]" },
    { 0x264D, "[Virgo]" },
    { 0x264E, "[Libra]" },
    { 0x264F, "[Scorpio]" },
    { 0x2650, "[Sagittarius]" },
    { 0x2651, "[Capricorn]" },
    { 0x2652, "[Aquarius]" },
    { 0x2653, "[Pisces]" },

    /* Music */
    { 0x1F3B5, "[music]" },   /* MUSICAL NOTE */
    { 0x1F3B6, "[music]" },   /* MULTIPLE MUSICAL NOTES */
    { 0x1F3A4, "[mic]" },     /* MICROPHONE */
    { 0x1F3A7, "[headphones]" }, /* HEADPHONE */

    /* Transport */
    { 0x1F680, "[rocket]" },  /* ROCKET */
    { 0x1F681, "[helicopter]" }, /* HELICOPTER */
    { 0x1F682, "[train]" },   /* STEAM LOCOMOTIVE */
    { 0x1F683, "[train]" },   /* RAILWAY CAR */
    { 0x1F684, "[train]" },   /* HIGH-SPEED TRAIN */
    { 0x1F685, "[train]" },   /* HIGH-SPEED TRAIN WITH BULLET NOSE */
    { 0x1F68C, "[bus]" },     /* BUS */
    { 0x1F691, "[ambulance]" },/* AMBULANCE */
    { 0x1F692, "[firetruck]" },/* FIRE ENGINE */
    { 0x1F693, "[police car]" },/* POLICE CAR */
    { 0x1F695, "[taxi]" },    /* TAXI */
    { 0x1F697, "[car]" },     /* AUTOMOBILE */
    { 0x1F699, "[SUV]" },     /* RECREATIONAL VEHICLE */
    { 0x1F69A, "[truck]" },   /* DELIVERY TRUCK */
    { 0x1F6A2, "[ship]" },    /* SHIP */
    { 0x1F6A4, "[boat]" },    /* SPEEDBOAT */
    { 0x1F6B2, "[bike]" },    /* BICYCLE */
    { 0x1F6E9, "[plane]" },   /* SMALL AIRPLANE */
    { 0x1F6EB, "[takeoff]" }, /* AIRPLANE DEPARTURE */
    { 0x1F6EC, "[landing]" }, /* AIRPLANE ARRIVING */
    { 0x1F6F0, "[satellite]" },/* SATELLITE */
    { 0x2708, "[plane]" },    /* AIRPLANE */

    /* Newer Unicode 7.0+ specific */
    { 0x1F910, ":|" },        /* ZIPPER-MOUTH FACE */
    { 0x1F911, "$)" },        /* MONEY-MOUTH FACE */
    { 0x1F912, ":(" },        /* FACE WITH THERMOMETER */
    { 0x1F913, "8)" },        /* NERD FACE */
    { 0x1F914, ":?" },        /* THINKING FACE */
    { 0x1F915, ":(" },        /* FACE WITH HEAD-BANDAGE */
    { 0x1F916, "[robot]" },   /* ROBOT FACE */
    { 0x1F917, ":)" },        /* HUGGING FACE */
    { 0x1F920, ":)" },        /* COWBOY HAT FACE */
    { 0x1F921, ":o)" },       /* CLOWN FACE */
    { 0x1F922, ":S" },        /* NAUSEATED FACE */
    { 0x1F923, "XD" },        /* ROLLING ON THE FLOOR LAUGHING */
    { 0x1F924, ":P" },        /* DROOLING FACE */
    { 0x1F925, ":>" },        /* LYING FACE */
    { 0x1F926, "[facepalm]" },/* FACE PALM */
    { 0x1F927, "[sneeze]" },  /* SNEEZING FACE */
    { 0x1F928, "8|" },        /* FACE WITH ONE EYEBROW RAISED */
    { 0x1F929, "*_*" },       /* STAR-STRUCK */
    { 0x1F92A, ":P" },        /* ZANY FACE */
    { 0x1F92B, ":X" },        /* SHUSHING FACE */
    { 0x1F92C, ">:(" },       /* FACE WITH SYMBOLS ON MOUTH */
    { 0x1F92D, ":X" },        /* FACE WITH HAND OVER MOUTH */
    { 0x1F92E, ":S" },        /* FACE VOMITING */
    { 0x1F92F, "[mind blown]" }, /* EXPLODING HEAD */
    { 0x1F970, ":)" },        /* SMILING FACE WITH HEARTS */
    { 0x1F973, "[party]" },   /* PARTYING FACE */
    { 0x1F974, ":S" },        /* WOOZY FACE */
    { 0x1F975, ":(" },        /* HOT FACE */
    { 0x1F976, ":(" },        /* COLD FACE */
    { 0x1F97A, ":'(" },       /* PLEADING FACE */

    /* Box drawing fallbacks - these should be in Unicode 6 but just in case */
    { 0x2500, "-" },          /* BOX DRAWINGS LIGHT HORIZONTAL */
    { 0x2502, "|" },          /* BOX DRAWINGS LIGHT VERTICAL */
    { 0x250C, "+" },          /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    { 0x2510, "+" },          /* BOX DRAWINGS LIGHT DOWN AND LEFT */
    { 0x2514, "+" },          /* BOX DRAWINGS LIGHT UP AND RIGHT */
    { 0x2518, "+" },          /* BOX DRAWINGS LIGHT UP AND LEFT */
    { 0x251C, "+" },          /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    { 0x2524, "+" },          /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    { 0x252C, "+" },          /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    { 0x2534, "+" },          /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    { 0x253C, "+" },          /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
};

#define NUM_FALLBACKS (sizeof(fallbacks) / sizeof(fallbacks[0]))

/* Linear search for fallback (table is small enough) */
static const char *find_fallback(uint32_t codepoint) {
    for (size_t i = 0; i < NUM_FALLBACKS; i++) {
        if (fallbacks[i].codepoint == codepoint) {
            return fallbacks[i].replacement;
        }
    }
    return NULL;
}

/* Decode UTF-8 sequence, return codepoint and advance pointer */
static uint32_t utf8_decode(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    uint32_t cp;
    int len;

    if (s[0] < 0x80) {
        cp = s[0];
        len = 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        cp = s[0] & 0x1F;
        len = 2;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = s[0] & 0x0F;
        len = 3;
    } else if ((s[0] & 0xF8) == 0xF0) {
        cp = s[0] & 0x07;
        len = 4;
    } else {
        /* Invalid UTF-8, skip byte */
        *p += 1;
        return 0xFFFD;  /* Replacement character */
    }

    for (int i = 1; i < len; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            /* Invalid continuation byte */
            *p += 1;
            return 0xFFFD;
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }

    *p += len;
    return cp;
}

/* Encode codepoint as UTF-8, return bytes written */
static int utf8_encode(uint32_t cp, char *out) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp < 0x110000) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

/* Check if codepoint is likely unsupported in Unicode 6.0 */
static int needs_fallback(uint32_t cp) {
    /* Emoji ranges (mostly Unicode 6.0+, but many added later) */
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return 1;  /* Misc Symbols and Pictographs + Emoticons + ... */
    if (cp >= 0x1FA00 && cp <= 0x1FAFF) return 1;  /* Chess, symbols, etc (Unicode 11+) */

    /* Extended-A emoji */
    if (cp >= 0x1F1E0 && cp <= 0x1F1FF) return 1;  /* Regional indicators */

    /* Some other ranges that may have issues */
    if (cp >= 0x2600 && cp <= 0x26FF) {
        /* Misc symbols - some are fine, check fallback table */
        return find_fallback(cp) != NULL;
    }
    if (cp >= 0x2700 && cp <= 0x27BF) {
        /* Dingbats - some are fine, check fallback table */
        return find_fallback(cp) != NULL;
    }
    if (cp >= 0x2B00 && cp <= 0x2BFF) {
        /* Misc symbols and arrows */
        return find_fallback(cp) != NULL;
    }

    return 0;
}

char *unicode_sanitize(const char *text) {
    if (!text) return NULL;

    /* Allocate output buffer - may need more space for expansions */
    size_t len = strlen(text);
    size_t out_capacity = len * 2 + 1;  /* Worst case: every char expands */
    char *out = malloc(out_capacity);
    if (!out) return NULL;

    const char *p = text;
    char *q = out;

    while (*p) {
        const char *start = p;
        uint32_t cp = utf8_decode(&p);

        if (cp == 0xFFFD) {
            /* Invalid UTF-8, copy replacement char */
            *q++ = '?';
            continue;
        }

        /* Check if this codepoint needs fallback */
        const char *fallback = find_fallback(cp);
        if (fallback) {
            /* Use fallback string */
            size_t fb_len = strlen(fallback);
            /* Check buffer space */
            size_t used = q - out;
            if (used + fb_len + 1 > out_capacity) {
                out_capacity = out_capacity * 2 + fb_len;
                char *new_out = realloc(out, out_capacity);
                if (!new_out) {
                    free(out);
                    return NULL;
                }
                q = new_out + used;
                out = new_out;
            }
            memcpy(q, fallback, fb_len);
            q += fb_len;
        } else if (needs_fallback(cp) && !fallback) {
            /* Unknown emoji/symbol without specific fallback */
            /* Use generic placeholder based on range */
            const char *generic;
            if (cp >= 0x1F600 && cp <= 0x1F64F) {
                generic = ":)";  /* Emoticons */
            } else if (cp >= 0x1F300 && cp <= 0x1F33F) {
                generic = "*";   /* Weather/nature symbols */
            } else if (cp >= 0x1F340 && cp <= 0x1F37F) {
                generic = "*";   /* Plants and food */
            } else if (cp >= 0x1F380 && cp <= 0x1F3FF) {
                generic = "*";   /* Celebrations/objects */
            } else if (cp >= 0x1F400 && cp <= 0x1F43F) {
                generic = "*";   /* Animals */
            } else if (cp >= 0x1F440 && cp <= 0x1F4FF) {
                generic = "*";   /* Body parts and objects */
            } else if (cp >= 0x1F500 && cp <= 0x1F5FF) {
                generic = "*";   /* Symbols */
            } else if (cp >= 0x1F680 && cp <= 0x1F6FF) {
                generic = "*";   /* Transport/maps */
            } else if (cp >= 0x1F900 && cp <= 0x1F9FF) {
                generic = ":)";  /* Supplemental emoticons */
            } else if (cp >= 0x1FA00 && cp <= 0x1FAFF) {
                generic = "*";   /* Extended symbols */
            } else {
                generic = "*";   /* Other symbols */
            }
            size_t g_len = strlen(generic);
            size_t used = q - out;
            if (used + g_len + 1 > out_capacity) {
                out_capacity = out_capacity * 2 + g_len;
                char *new_out = realloc(out, out_capacity);
                if (!new_out) {
                    free(out);
                    return NULL;
                }
                q = new_out + used;
                out = new_out;
            }
            memcpy(q, generic, g_len);
            q += g_len;
        } else {
            /* Keep original character */
            size_t char_len = p - start;
            size_t used = q - out;
            if (used + char_len + 1 > out_capacity) {
                out_capacity = out_capacity * 2 + char_len;
                char *new_out = realloc(out, out_capacity);
                if (!new_out) {
                    free(out);
                    return NULL;
                }
                q = new_out + used;
                out = new_out;
            }
            memcpy(q, start, char_len);
            q += char_len;
        }
    }

    *q = '\0';

    /* Shrink to fit */
    size_t final_len = q - out + 1;
    char *final = realloc(out, final_len);
    return final ? final : out;
}
