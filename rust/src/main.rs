#![feature(portable_simd)]
use anyhow::Result;
use clap::Parser;
use std::io::BufRead;
use std::path::Path;
use std::simd::cmp::SimdPartialEq;
use std::simd::num::SimdUint;
use std::sync::mpsc;

const K0: u32 = 0x12345678;
const K1: u32 = 0x23456789;
const K2: u32 = 0x34567890;
const MAX_CHECKS: u64 = 50_000_000;
static ALL_CHARS: &[u8] = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

type Batch = std::simd::u32x16;
type Mask = std::simd::mask32x16;

// Non-SIMD CRC calculation.
fn crc32(x: u32, c: u8) -> u32 {
    let idx: usize = ((x as u8) ^ c) as usize;
    (x >> 8) ^ CRCTAB[idx]
}

// SIMD CRC calculation.
fn crc32_simd(x: Batch, c: Batch) -> Batch {
    let ff = Batch::splat(0xff);
    let idx = (x ^ c) & ff;
    let idx = idx.cast::<usize>();
    (x >> 8) ^ Batch::gather_or_default(&CRCTAB, idx)
}

const CRCTAB: [u32; 256] = [
    0, 1996959894, 3993919788, 2567524794, 124634137, 1886057615, 3915621685, 2657392035,
    249268274, 2044508324, 3772115230, 2547177864, 162941995, 2125561021, 3887607047, 2428444049,
    498536548, 1789927666, 4089016648, 2227061214, 450548861, 1843258603, 4107580753, 2211677639,
    325883990, 1684777152, 4251122042, 2321926636, 335633487, 1661365465, 4195302755, 2366115317,
    997073096, 1281953886, 3579855332, 2724688242, 1006888145, 1258607687, 3524101629, 2768942443,
    901097722, 1119000684, 3686517206, 2898065728, 853044451, 1172266101, 3705015759, 2882616665,
    651767980, 1373503546, 3369554304, 3218104598, 565507253, 1454621731, 3485111705, 3099436303,
    671266974, 1594198024, 3322730930, 2970347812, 795835527, 1483230225, 3244367275, 3060149565,
    1994146192, 31158534, 2563907772, 4023717930, 1907459465, 112637215, 2680153253, 3904427059,
    2013776290, 251722036, 2517215374, 3775830040, 2137656763, 141376813, 2439277719, 3865271297,
    1802195444, 476864866, 2238001368, 4066508878, 1812370925, 453092731, 2181625025, 4111451223,
    1706088902, 314042704, 2344532202, 4240017532, 1658658271, 366619977, 2362670323, 4224994405,
    1303535960, 984961486, 2747007092, 3569037538, 1256170817, 1037604311, 2765210733, 3554079995,
    1131014506, 879679996, 2909243462, 3663771856, 1141124467, 855842277, 2852801631, 3708648649,
    1342533948, 654459306, 3188396048, 3373015174, 1466479909, 544179635, 3110523913, 3462522015,
    1591671054, 702138776, 2966460450, 3352799412, 1504918807, 783551873, 3082640443, 3233442989,
    3988292384, 2596254646, 62317068, 1957810842, 3939845945, 2647816111, 81470997, 1943803523,
    3814918930, 2489596804, 225274430, 2053790376, 3826175755, 2466906013, 167816743, 2097651377,
    4027552580, 2265490386, 503444072, 1762050814, 4150417245, 2154129355, 426522225, 1852507879,
    4275313526, 2312317920, 282753626, 1742555852, 4189708143, 2394877945, 397917763, 1622183637,
    3604390888, 2714866558, 953729732, 1340076626, 3518719985, 2797360999, 1068828381, 1219638859,
    3624741850, 2936675148, 906185462, 1090812512, 3747672003, 2825379669, 829329135, 1181335161,
    3412177804, 3160834842, 628085408, 1382605366, 3423369109, 3138078467, 570562233, 1426400815,
    3317316542, 2998733608, 733239954, 1555261956, 3268935591, 3050360625, 752459403, 1541320221,
    2607071920, 3965973030, 1969922972, 40735498, 2617837225, 3943577151, 1913087877, 83908371,
    2512341634, 3803740692, 2075208622, 213261112, 2463272603, 3855990285, 2094854071, 198958881,
    2262029012, 4057260610, 1759359992, 534414190, 2176718541, 4139329115, 1873836001, 414664567,
    2282248934, 4279200368, 1711684554, 285281116, 2405801727, 4167216745, 1634467795, 376229701,
    2685067896, 3608007406, 1308918612, 956543938, 2808555105, 3495958263, 1231636301, 1047427035,
    2932959818, 3654703836, 1088359270, 936918000, 2847714899, 3736837829, 1202900863, 817233897,
    3183342108, 3401237130, 1404277552, 615818150, 3134207493, 3453421203, 1423857449, 601450431,
    3009837614, 3294710456, 1567103746, 711928724, 3020668471, 3272380065, 1510334235, 755167117,
];

#[derive(Clone, Copy)]
enum Engine {
    Simd,
    Plain,
}

#[derive(Clone)]
enum Generator {
    LowerCase(u8),
    Pattern(String),
}

// Non-SIMD key state. Updated every password character.
#[derive(Default, Clone, Copy)]
struct KeyState {
    key: [u32; 3],
    key3: u8,
}

impl KeyState {
    fn new(pw: &[u8]) -> Self {
        let mut ks = Self {
            key: [K0, K1, K2],
            key3: 0,
        };
        for ch in pw {
            ks.updatekeys(*ch);
        }
        ks
    }
    fn updatekeys(&mut self, byte: u8) {
        self.key[0] = crc32(self.key[0], byte);
        let t = self.key[1] as u64 + (self.key[0] & 0xff) as u64;
        let t = t * 134775813 + 1;
        self.key[1] = t as u32;
        self.key[2] = crc32(self.key[2], ((self.key[1] >> 24) & 0xff) as u8);
        let temp = (self.key[2] & 0xffff) | 3;
        self.key3 = (((temp * (temp ^ 1)) >> 8) & 0xff) as u8;
    }
}

trait PWGen {
    /// Generate the next password.
    fn next(&mut self) -> (&[u8], bool, bool);

    /// Get the password in string form.
    fn to_string(&self) -> String;

    /// Get the possible characters for the last position.
    fn last_char(&self) -> Vec<u8>;
}

struct PWGenLowerCase {
    state: Vec<u8>,
}

impl PWGenLowerCase {
    fn new(n: u8) -> Self {
        let mut state = vec![b'a'; n as usize];
        state[n as usize - 1] -= 1;
        Self { state }
    }
    fn prefix(&self) -> Self {
        Self {
            state: self.state[0..self.state.len() - 1].to_vec(),
        }
    }
}

impl PWGen for PWGenLowerCase {
    fn next(&mut self) -> (&[u8], bool, bool) {
        // TODO: this actually skips the first password, so the
        // password had better not be "aaaaaa".
        let mut pos = self.state.len() - 1;
        let mut change = false;
        self.state[pos] += 1;
        while self.state[pos] > b'z' {
            if pos == 0 {
                return (&self.state, true, true);
            }
            self.state[pos] = b'a';
            change = true;
            pos -= 1;
            self.state[pos] += 1;
        }
        (&self.state, change, false)
    }
    fn to_string(&self) -> String {
        String::from_utf8(self.state.clone()).unwrap()
    }
    fn last_char(&self) -> Vec<u8> {
        let mut ret = vec![];
        for ch in ALL_CHARS {
            ret.push(*ch);
        }
        ret
    }
}

// Pattern is meant to allow more than just a period for wildcard, but
// not implemented yet.
struct PWGenPattern {
    pattern: String,
    state: Vec<u8>,
    nxt: Vec<u8>,
    chars: Vec<Vec<u8>>,
}

impl PWGenPattern {
    fn new(pattern: &str) -> Self {
        let n = pattern.len();
        let mut chars = Vec::with_capacity(n);
        let mut nxt = Vec::with_capacity(n);
        for ch in pattern.chars() {
            match ch {
                '.' => {
                    chars.push(ALL_CHARS.to_vec());
                    nxt.push(ALL_CHARS[0]);
                }
                fixed => {
                    chars.push(vec![fixed as u8]);
                    nxt.push(fixed as u8);
                }
            }
        }
        Self {
            pattern: pattern.to_string(),
            state: vec![0; n],
            nxt,
            chars,
        }
    }
    fn prefix(&self) -> Self {
        PWGenPattern::new(&self.pattern[..self.pattern.len() - 1])
    }
}

impl PWGen for PWGenPattern {
    fn next(&mut self) -> (&[u8], bool, bool) {
        let len = self.state.len();
        let mut pos = len - 1;
        let mut change = false;
        if self.state.iter().all(|x| *x == 0) {
            change = true;
        }
        self.state[pos] += 1;
        while self.state[pos] >= self.chars[pos].len() as u8 {
            if pos == 0 {
                return (&self.state, true, true);
            }
            self.state[pos] = 0;
            change = true;
            pos -= 1;
            self.state[pos] += 1;
        }
        for i in pos..len {
            self.nxt[i] = self.chars[i][self.state[i] as usize];
        }
        (&self.nxt, change, false)
    }
    fn to_string(&self) -> String {
        String::from_utf8(self.nxt.clone()).unwrap()
    }
    fn last_char(&self) -> Vec<u8> {
        self.chars[self.chars.len() - 1].clone()
    }
}

// Non-SIMD crack function.
fn crack(
    filedata: &[&[u8]],
    fileheaders: &[u8],
    mut password: impl PWGen,
    candidate_tx: std::sync::mpsc::Sender<String>,
) -> u64 {
    let mut ks = KeyState::default();
    ks.updatekeys(0);
    let mut n = 0_u64;
    let mut ckey = KeyState::default();

    'next: loop {
        n += 1;
        let (pw, change, done) = password.next();
        if done {
            return n;
        }
        //eprintln!("Testing {pw:?}");
        if n > MAX_CHECKS {
            return n;
        }
        if change {
            ks.key[0] = K0;
            ks.key[1] = K1;
            ks.key[2] = K2;
            for ch in &pw[..pw.len() - 1] {
                ks.updatekeys(*ch);
            }
            ckey = ks;
        } else {
            ks = ckey;
        }
        {
            let ch = pw[pw.len() - 1];
            ks.updatekeys(ch);
        }
        for nfile in 0..filedata.len() {
            let mut tkey = ks;
            for t in &filedata[nfile][0..11] {
                tkey.updatekeys(t ^ tkey.key3);
            }
            if fileheaders[nfile] != filedata[nfile][11] ^ tkey.key3 {
                //eprintln!("No: {:?}", String::from_utf8(password.clone()));
                continue 'next;
            }
        }
        candidate_tx
            .send(password.to_string())
            .expect("candidate failed to send");
        eprintln!("Maybe yes: {:?}", password.to_string());
    }
}

// SIMD password state. Updated after every password character.
#[derive(Default, Clone, Copy)]
struct SKeyState {
    key0: Batch,
    key1: Batch,
    key2: Batch,
    key3: Batch,
}

impl SKeyState {
    fn update(&mut self, byte: Batch) {
        // Consts.
        let magic = Batch::splat(134775813);
        let one = Batch::splat(1);
        let ff = Batch::splat(0xff);
        let ffff = Batch::splat(0xffff);
        let three = Batch::splat(3);

        // table lookup
        self.key0 = crc32_simd(self.key0, byte);
        let t = self.key1 + (self.key0 & ff);
        self.key1 = t * magic + one;
        self.key2 = crc32_simd(self.key2, (self.key1 >> 24) & ff);
        let temp = (self.key2 & ffff) | three;
        self.key3 = ((temp * (temp ^ one)) >> 8) & ff;
    }
}

// SIMD password cracker.
//
// TODO: the last char is so cheap to check, that it's probably faster
// to Simd over the *second* to last character instead.
fn crack_simd(
    filedata: &[&[u8]],
    fileheaders: &[u8],
    mut password: impl PWGen,
    last_chars: &[u8],
    candidate_tx: std::sync::mpsc::Sender<String>,
) -> u64 {
    let mut n = 0_u64;
    let mut ckey = KeyState::default();
    let (lasts, lastlen) = {
        let l = last_chars;
        let mut ret = vec![];
        for i in (0..l.len()).step_by(16) {
            let mut b = vec![];
            for j in 0..16 {
                if i + j >= l.len() {
                    b.push(0); // Waste.
                } else {
                    b.push(l[i + j] as u32);
                }
            }
            ret.push(Batch::from_slice(&b));
        }
        (ret, l.len() as u64)
    };

    loop {
        let (pw, change, done) = password.next();
        //println!("pw: {pw:?}");
        if done {
            return n;
        }
        if n > MAX_CHECKS {
            return n;
        }
        // Use cache for second to last.
        if change {
            let cache_pw = &pw[..pw.len() - 1];
            //println!("Cache entry: {cache_pw:?}");
            ckey = KeyState::new(cache_pw);
        }
        let mut ks = ckey;

        // Add second to last character.
        {
            let second_to_last = pw[pw.len() - 1];
            //println!("Adding second to last: {second_to_last}");
            ks.updatekeys(second_to_last);
        }

        n += lastlen;
        // Simd the last character.
        //println!("Running last batch");
        'nextlast: for last in &lasts {
            //println!("Running a batch of {}: {last:?}", last.len());
            let mut state = SKeyState {
                key0: Batch::splat(ks.key[0]),
                key1: Batch::splat(ks.key[1]),
                key2: Batch::splat(ks.key[2]),
                key3: Batch::splat(0),
            };
            state.update(*last);

            let mut ok = Mask::splat(true);
            for nfile in 0..filedata.len() {
                let mut tkey = state;
                for t in &filedata[nfile][0..11] {
                    let t = Batch::splat(*t as u32);
                    tkey.update(t ^ tkey.key3);
                }
                let correct = Batch::splat(fileheaders[nfile].into());
                let last = Batch::splat(filedata[nfile][11].into());
                let got = last ^ tkey.key3;
                ok &= got.simd_eq(correct);
                if !ok.any() {
                    continue 'nextlast;
                }
                //println!("{ok:?}");
            }

            // It's OK that this is slow, since at least one password
            // matched, which should be rare.
            let candidates: Vec<_> = ok
                .to_array()
                .iter()
                .enumerate()
                .filter(|(_idx, &val)| val)
                .map(|(idx, _)| {
                    let ch = (b'a' + idx as u8) as char;
                    let prefix = std::str::from_utf8(pw).unwrap();
                    let mut s = prefix.to_string();
                    s.push(ch);
                    candidate_tx
                        .send(s.clone())
                        .expect("candidate failed to send");
                    s
                })
                .collect();
            println!("candidates {candidates:?}");
        }
    }
}

// Load relevant zip file data. We don't need the whole zip file.
//
// The format is:
// * One row per file in the zip archive.
// * Every line has 13 hex numbers.
// * First the `head` byte.
// * The rest is file data.
fn load_file_data(path: &Path) -> Result<(Vec<[u8; 12]>, Vec<u8>)> {
    let file = std::fs::File::open(path)?;
    let reader = std::io::BufReader::new(file);
    let mut data = vec![];
    let mut head = vec![];

    for line in reader.lines() {
        let u8s: Vec<_> = line?
            .split(' ')
            .filter_map(|s| u8::from_str_radix(s, 16).ok())
            .collect();
        if u8s.is_empty() {
            continue;
        }
        if u8s.len() != 13 {
            return Err(anyhow::anyhow!(
                "zipdata format error: every line should have 13 hex u8's"
            ));
        }
        let mut t = [0_u8; 12];
        t.copy_from_slice(&u8s[1..13]);
        data.push(t);
        head.push(u8s[0]);
    }
    Ok((data, head))
}

#[derive(Parser, Debug)]
struct Opt {
    #[clap(short, long)]
    pattern: Option<String>,

    #[clap(short, long)]
    length: Option<u8>,

    #[clap(short, long, default_value = "1")]
    threads: usize,

    #[clap(short)]
    filename: String,

    #[clap(long, default_value = "simd")]
    engine: String,
}

fn main() -> Result<()> {
    let opt = Opt::parse();

    let engine = match opt.engine.to_lowercase().as_str() {
        "plain" => Engine::Plain,
        "simd" => Engine::Simd,
        _ => panic!("Only engines are SIMD and plain"),
    };
    let generator = match (opt.length, opt.pattern) {
        (Some(_), Some(_)) => panic!("Can't provide both length and pattern generators."),
        (Some(n), None) => Generator::LowerCase(n),
        (None, Some(p)) => Generator::Pattern(p),
        (None, None) => panic!("Need either length or pattern."),
    };

    let (candidate_tx, candidate_rx) = mpsc::channel();

    let (filedata, fileheaders) = load_file_data(Path::new(&opt.filename))?;
    assert_eq!(filedata.len(), fileheaders.len());

    if false {
        // Simple test version, hard coding stuff.
        let filedata = filedata.clone();
        let filedata: &Vec<&[u8]> = &filedata.iter().map(AsRef::as_ref).collect();
        let gen = PWGenLowerCase::new(2);
        crack_simd(
            filedata,
            &fileheaders,
            gen.prefix(),
            &gen.last_char(),
            candidate_tx.clone(),
        );
        return Ok(());
    }

    // Let's roll.
    use std::time::Instant;
    let start = Instant::now();
    let cpu_start = cpu_time::ProcessTime::try_now().expect("Getting process time failed");

    let mut entries: u64 = 0;
    if opt.threads == 1 {
        let filedata: &Vec<&[u8]> = &filedata.iter().map(AsRef::as_ref).collect();
        entries = match (engine, generator) {
            (Engine::Simd, Generator::LowerCase(n)) => {
                let gen = PWGenLowerCase::new(n);
                crack_simd(
                    filedata,
                    &fileheaders,
                    gen.prefix(),
                    &gen.last_char(),
                    candidate_tx.clone(),
                )
            }
            (Engine::Simd, Generator::Pattern(s)) => {
                let gen = PWGenPattern::new(&s);
                crack_simd(
                    filedata,
                    &fileheaders,
                    gen.prefix(),
                    &gen.last_char(),
                    candidate_tx.clone(),
                )
            }
            (Engine::Plain, Generator::LowerCase(n)) => {
                let gen = PWGenLowerCase::new(n);
                crack(filedata, &fileheaders, gen, candidate_tx.clone())
            }
            (Engine::Plain, Generator::Pattern(s)) => {
                let gen = PWGenPattern::new(&s);
                crack(filedata, &fileheaders, gen, candidate_tx.clone())
            }
        };
    } else {
        use std::sync::{Arc, Mutex};
        let (tx, rx) = mpsc::channel();
        let rx = Arc::new(Mutex::new(rx));
        match generator {
            Generator::Pattern(ref p) => {
                assert_eq!(p.chars().next(), Some('.'));
                //println!("Thread task count: {}", ALL_CHARS.len());
                for pre in ALL_CHARS {
                    let pat = format!("{}{}", *pre as char, &p[1..]);
                    //println!("Sent '{pat}'");
                    tx.send(pat).expect("send password");
                }
            }
            Generator::LowerCase(n) => {
                let pattern = ".".repeat((n - 1).into());
                // TODO: this is inconsistent. Threaded it actually
                // does *ALL* characters. Should only do lower case.
                //
                // But pattern currently doesn't support that.
                for pre in ALL_CHARS {
                    let pat = format!("{}{pattern}", *pre as char);
                    //println!("{pat}");
                    tx.send(pat).expect("send password");
                }
            }
        };
        drop(tx);
        let mut threads = vec![];
        for _ in 0..opt.threads {
            let rx2 = Arc::clone(&rx);
            let filedata = filedata.clone();
            let fileheaders = fileheaders.clone();
            let candidate_tx = candidate_tx.clone();
            threads.push(std::thread::spawn(move || {
                let filedata: &Vec<&[u8]> = &filedata.iter().map(AsRef::as_ref).collect();
                let mut sum = 0;
                loop {
                    let msg = rx2.lock().unwrap().recv();
                    match msg {
                        Ok(pw) => match engine {
                            Engine::Simd => {
                                let gen = PWGenPattern::new(&pw);
                                let gen2 = gen.prefix();
                                //println!("generator {:?} with last char {:?}", gen2.pattern, gen.last_char());
                                sum += crack_simd(
                                    filedata,
                                    &fileheaders,
                                    gen2,
                                    &gen.last_char(),
                                    candidate_tx.clone(),
                                )
                            }
                            Engine::Plain => {
                                sum += crack(
                                    filedata,
                                    &fileheaders,
                                    PWGenPattern::new(&pw),
                                    candidate_tx.clone(),
                                );
                            }
                        },
                        Err(_) => {
                            return sum;
                        }
                    };
                }
            }));
        }
        for t in threads {
            entries += t.join().expect("thread failed");
        }
    }
    drop(candidate_tx);
    for c in candidate_rx {
        println!("Candidate: {c}");
    }
    let cputime = cpu_start.elapsed().as_secs_f32();
    let walltime = start.elapsed().as_secs_f32();
    let mentries = entries as f32 / 1_000_000.0;
    println!("Passwords checked: {entries}");
    println!("Elapsed time:      {walltime} s");
    println!("CPU time:          {cputime} s");
    println!("Parallelism:       {}", cputime / walltime);
    println!("Passwords/CPU-sec: {:.3} M", mentries / cputime);
    println!("Passwords/wallsec  {:.3} M", mentries / walltime);
    println!(
        "Theoretical speed: {:.3} M",
        mentries / cputime * opt.threads as f32
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn crack_test_plain(filename: &str, gen: impl PWGen) -> Result<Vec<String>> {
        let (d, h) = load_file_data(Path::new(filename))?;
        let d: &Vec<&[u8]> = &d.iter().map(AsRef::as_ref).collect();
        let (tx, rx) = mpsc::channel();
        crack(d, &h, gen, tx);
        let mut cs = vec![];
        for pw in rx {
            cs.push(pw);
        }
        Ok(cs)
    }

    #[test]
    fn crack_hej_length() -> Result<()> {
        let cs = crack_test_plain("zipdata.txt", PWGenLowerCase::new(3))?;
        assert_eq!(cs, ["hej"]);
        Ok(())
    }

    #[test]
    fn crack_hej_fail() -> Result<()> {
        let cs = crack_test_plain("zipdata.txt", PWGenLowerCase::new(2))?;
        assert!(cs.is_empty());
        Ok(())
    }
}
