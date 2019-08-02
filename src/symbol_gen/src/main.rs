use std::{error::Error, fs, io::Write, io::BufWriter, path::PathBuf};

use goblin::elf::sym;
use structopt::StructOpt;

#[derive(StructOpt, Debug)]
struct Opt {
    #[structopt(
        short = "i",
        long = "input",
        parse(from_os_str),
        default_value = "../wisp5-bootloader/Debug/wisp5-bootloader.out"
    )]
    input: PathBuf,

    #[structopt(
        short = "o",
        long = "output",
        parse(from_os_str),
        default_value = "./bootloader_symbols.cmd"
    )]
    output: PathBuf,
}

fn main() {
    let opt = Opt::from_args();
    if let Err(e) = run(&opt) {
        eprintln!("{}", e);
        std::process::exit(1);
    }
}

fn run(opt: &Opt) -> Result<(), Box<dyn Error>> {
    // Parse the input as an elf file
    let data = fs::read(&opt.input)?;
    let elf = match goblin::Object::parse(&data)? {
        goblin::Object::Elf(elf) => elf,
        _ => return Err("Input was not an elf file".into()),
    };

    // Dump all the symbols to a linker cmd file
    let mut output_file = BufWriter::new(fs::File::create(&opt.output)?);
    for symbol in &elf.syms {
        let name = &elf.strtab[symbol.st_name];

        // Ignore certain types of symbols:
        //  - Local symbols
        //  - C runtime symbols
        //  - Intrinsic functions (starting with _)
        const C_RUNTIME_SYM: &[&str] = &["main", "C$$EXIT", "abort"];
        if symbol.st_bind() != sym::STB_GLOBAL
            || C_RUNTIME_SYM.contains(&name)
            || name.starts_with("_")
        {
            continue;
        }

        let addr = symbol.st_value;
        match symbol.st_type() {
            sym::STT_COMMON | sym::STT_FUNC | sym::STT_OBJECT => {
                writeln!(output_file, "{}=0x{:X};", name, addr)?
            }
            _ => {}
        }
    }

    Ok(())
}
