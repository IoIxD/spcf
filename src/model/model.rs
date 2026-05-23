use std::{ffi::CStr, os::raw::c_char, ptr::null_mut};

use candle_nn::{Func, Module, VarBuilder};
use candle_transformers::models::{
    convnext,
    mimi::candle::{D, DType, Device, IndexOp},
    resnet,
};

struct ModelContext {
    device: Device,
    model: *mut Func<'static>,
    scanned_names: [[c_char; 255]; 255],
}
#[unsafe(no_mangle)]
unsafe extern "C" fn model_new(error_str: *mut c_char, error_str_len: usize) -> *mut ModelContext {
    match || -> Result<*mut ModelContext, Box<dyn std::error::Error>> {
        let no_cpu = std::env::var("SORTMANCER_CONVNEXT").is_ok();
        let device = match candle_examples::device(false) {
            Ok(device) => device,
            Err(err) => {
                if std::env::var("SORTMANCER_CONVNEXT").is_ok() {
                    println!("Couldn't select GPU: {}", err.to_string());
                    Device::Cpu
                } else {
                    return Err(Box::new(err));
                }
            }
        };
        if device.is_cpu() && !no_cpu {
            println!("using cpu model");
            let api = hf_hub::api::sync::Api::new()?;
            let api = api.model("lmz/candle-resnet".into());
            let filename = api.get("resnet152.safetensors")?;
            let vb =
                unsafe { VarBuilder::from_mmaped_safetensors(&[filename], DType::F32, &device)? };
            let model = resnet::resnet152(candle_examples::imagenet::CLASS_COUNT as usize, vb)?;

            Ok(Box::leak(Box::new(ModelContext {
                device,
                model: Box::leak(Box::new(model)),
                scanned_names: [[0; 255]; 255],
            })))
        } else {
            println!("using gpu model");
            let api = hf_hub::api::sync::Api::new()?;

            let api = api.model("timm/convnextv2_huge.fcmae_ft_in1k".into());
            let filename = api.get("model.safetensors")?;
            let vb =
                unsafe { VarBuilder::from_mmaped_safetensors(&[filename], DType::F32, &device)? };

            let config = convnext::Config::huge();
            let model = convnext::convnext(&config, 1000, vb)?;

            Ok(Box::leak(Box::new(ModelContext {
                device,
                model: Box::leak(Box::new(model)),
                scanned_names: [[0; 255]; 255],
            })))
        }
    }() {
        Ok(a) => a,
        Err(err) => {
            let error = err.to_string();
            let ptr = error.as_ptr() as *mut i8;
            let mut len = err.to_string().len();
            if len > error_str_len {
                len = error_str_len;
            }
            std::ptr::copy(ptr, error_str, len);
            null_mut()
        }
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn model_scan(
    model: *mut ModelContext,
    filename: *const c_char,
    error_str: *mut c_char,
    error_str_len: usize,
) -> bool {
    match || -> Result<(), Box<dyn std::error::Error>> {
        let rust_filename = CStr::from_ptr(filename).to_string_lossy().to_string();
        let image =
            candle_examples::imagenet::load_image224(rust_filename)?.to_device(&(*model).device)?;
        let m = model.as_mut().unwrap().model;
        (*model).scanned_names = [[0; 255]; 255];

        let logits = (*m).forward(&image.unsqueeze(0)?)?;
        let prs = candle_nn::ops::softmax(&logits, D::Minus1)?
            .i(0)?
            .to_vec1::<f32>()?;
        let mut prs = prs.iter().enumerate().collect::<Vec<_>>();
        prs.sort_by(|(_, p1), (_, p2)| p2.total_cmp(p1));
        let mut i = 0;

        for &(category_idx, _pr) in prs.iter().take(32) {
            let st = candle_examples::imagenet::CLASSES[category_idx];
            let error = st.to_string();
            let ptr = error.as_ptr() as *mut i8;
            let mut len = st.to_string().len();
            if len > 255 {
                len = 255;
            }
            std::ptr::copy(ptr, (*model).scanned_names[i].as_mut_ptr(), len);
            i += 1;
        }
        Ok(())
    }() {
        Ok(_) => true,
        Err(err) => {
            let error = err.to_string();
            let ptr = error.as_ptr() as *mut i8;
            let mut len = err.to_string().len();
            if len > error_str_len {
                len = error_str_len;
            }
            std::ptr::copy(ptr, error_str, len);
            false
        }
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn model_scanned_names_get_idx(
    model: *mut ModelContext,
    idx: usize,
    fuck: *mut c_char,
) {
    let error = (*model).scanned_names[idx];
    std::ptr::copy(error.as_ptr() as *mut i8, fuck, 255);
}
