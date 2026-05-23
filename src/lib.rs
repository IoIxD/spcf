#![allow(unsafe_op_in_unsafe_fn)] /* why is this even a thing in rust */

#[path = "model/model.rs"]
mod model;

#[path = "image/image.rs"]
mod image;
