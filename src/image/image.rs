use std::{
    ffi::CStr,
    os::raw::c_char,
    ptr::{null, null_mut},
};

use image::GenericImageView;

struct Image {
    pixels: Vec<u8>,
}

#[unsafe(no_mangle)]
extern "C" fn image_get(c_filename: *const c_char) -> *mut Image {
    let filename = unsafe { CStr::from_ptr(c_filename) }
        .to_string_lossy()
        .to_string();
    match image::open(filename.clone()) {
        Ok(img) => {
            let pixels = img
                .pixels()
                .map(|f| f.2.0)
                .flat_map(|f| f)
                .collect::<Vec<u8>>();
            return Box::leak(Box::new(Image { pixels }));
        }
        Err(err) => {
            println!("Couldn't read {}: {:?}", filename, err);
            return null_mut();
        }
    }
}
#[unsafe(no_mangle)]
unsafe extern "C" fn image_get_pixels(ptr: *mut Image) -> *const u8 {
    if ptr.is_null() {
        return null();
    }
    return (*ptr).pixels.as_ptr();
}
#[unsafe(no_mangle)]
unsafe extern "C" fn image_get_pixel_len(ptr: *mut Image) -> usize {
    if ptr.is_null() {
        return 0;
    }
    return (*ptr).pixels.len();
}

#[unsafe(no_mangle)]
unsafe extern "C" fn image_free(ptr: *mut Image) {
    let _ = Box::from_raw(ptr);
}
