// plain_numbers rewrites every JSON number written in exponent form (1e-05, 4.2E+03) as a plain decimal
// (0.00001, 4200) and leaves everything else - key order, strings, spacing - byte for byte as it was.
//
// Why (M1 test 2026-10-13): DayZ's own JsonSerializer wrote the player snapshot, but reading that exact text back
// failed with "Miss fraction part in number". Kept as a safety net (ported from the Go hub's numfix.go).

pub fn plain_numbers(s: &str) -> String {
    let b = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut in_str = false;
    let mut esc = false;
    let mut i = 0;
    while i < b.len() {
        let c = b[i];
        if in_str {
            // copy whole UTF-8 sequences untouched
            let ch = s[i..].chars().next().unwrap();
            out.push(ch);
            if esc {
                esc = false;
            } else if ch == '\\' {
                esc = true;
            } else if ch == '"' {
                in_str = false;
            }
            i += ch.len_utf8();
            continue;
        }
        if c == b'"' {
            in_str = true;
            out.push('"');
            i += 1;
            continue;
        }
        if c == b'-' || c.is_ascii_digit() {
            let mut j = i + 1;
            while j < b.len() && b"0123456789.eE+-".contains(&b[j]) {
                j += 1;
            }
            let mut num = s[i..j].to_string();
            if num.contains(['e', 'E']) {
                if let Ok(f) = num.parse::<f64>() {
                    num = format_plain(f);
                }
            }
            // "5." would also break DayZ: make sure a dot has digits after it
            if num.ends_with('.') {
                num.push('0');
            }
            out.push_str(&num);
            i = j;
            continue;
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

// shortest plain decimal that round-trips (like Go's strconv.FormatFloat(f, 'f', -1, 64))
fn format_plain(f: f64) -> String {
    let short = format!("{}", f); // Rust's Display never uses exponents and is round-trip shortest
    short
}

#[cfg(test)]
mod tests {
    use super::plain_numbers;

    #[test]
    fn plain() {
        let cases = [
            (r#"{"wet":1e-05,"h":0.4866,"q":-1,"n":"Apple 1e-05","w":4.2E+03,"z":0}"#, r#"{"wet":0.00001,"h":0.4866,"q":-1,"n":"Apple 1e-05","w":4200,"z":0}"#),
            (r#"{"a":[1.5e-7,-2e2],"s":"x\"1e5\""}"#, r#"{"a":[0.00000015,-200],"s":"x\"1e5\""}"#),
            (r#"{"t":5.}"#, r#"{"t":5.0}"#),
            (r#"{"keep":"order","b":2,"a":1}"#, r#"{"keep":"order","b":2,"a":1}"#),
            (r#"{"ü":"é 1e5","x":1e1}"#, r#"{"ü":"é 1e5","x":10}"#),
        ];
        for (input, want) in cases {
            assert_eq!(plain_numbers(input), want, "input {input}");
        }
    }
}
