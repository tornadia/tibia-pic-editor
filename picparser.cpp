#include <fstream>
#include <iomanip>
#include <iostream>
#include <png.h>
#include <sstream>
#include <cstring>
// #include <boost/filesystem/operations.hpp>
#include <sys/stat.h>
#include <unistd.h>
//#include <boost.h>
using namespace std;

#define DEBUG

struct tpixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

struct tsprite {
  tpixel **pixel;
};

struct timage {
  unsigned char width, height;
  unsigned char unk0, unk1, unk2;
  tsprite **sprite;
};

int write_png_image(FILE *pngfile, timage *image) {
  int width, height;
  png_byte color_type;
  png_byte bit_depth;

  png_bytep *row_pointers;

  // initialize stuff
  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr) {
    cout << "!png_ptr" << endl;
    return 1;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    cout << "!info_ptr" << endl;
    return 1;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    cout << "setjmp(png_jmpbuf(png_ptr))" << endl;
    return 1;
  }

  png_init_io(png_ptr, pngfile);

  // write header
  if (setjmp(png_jmpbuf(png_ptr))) {
    cout << "setjmp(png_jmpbuf(png_ptr))" << endl;
    return 1;
  }

  width = image->width * 32;
  height = image->height * 32;
  color_type = PNG_COLOR_TYPE_RGBA;
  bit_depth = 8;

  png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  // Fill row_pointers
  row_pointers = new png_bytep[height];
  for (int y = 0; y < height; y++)
    row_pointers[y] = new png_byte[png_get_rowbytes(png_ptr, info_ptr)];

  for (int y = 0; y < height; y++) {
    png_byte *row = row_pointers[y];
    for (int x = 0; x < width; x++) {
      png_byte *ptr = &(row[x * 4]);

      ptr[0] = image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].red;
      ptr[1] = image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].green;
      ptr[2] = image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].blue;
      ptr[3] = image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].alpha;
    }
  }

  // write bytes
  if (setjmp(png_jmpbuf(png_ptr))) {
    cout << "setjmp(png_jmpbuf(png_ptr))" << endl;
    return 1;
  }

  png_write_image(png_ptr, row_pointers);

  // end write
  if (setjmp(png_jmpbuf(png_ptr))) {
    cout << "setjmp(png_jmpbuf(png_ptr))" << endl;
    return 1;
  }

  png_write_end(png_ptr, NULL);

  // cleanup heap allocation
  for (int y = 0; y < height; y++)
    // free(row_pointers[y]);
    delete[] row_pointers[y];
  // free(row_pointers);
  delete[] row_pointers;

  // fclose(fp);
}

int read_sprite(ifstream *pic, int sprite_adr, tsprite *sprite) {
#ifdef DEBUG
      cout << "Reading sector data. " << endl;
#endif
  int adr;

  unsigned short int size, empty_pixels, pixel_num;

  adr = pic->tellg();

  pic->seekg(sprite_adr);

  pic->read((char *)&size, sizeof(size));
#ifdef DEBUG
      cout << "Reading size: " << size << endl;
#endif
  int byte_count = 0;
  int pixel_count = 0;
  while (byte_count < size) {
    pic->read((char *)&empty_pixels, sizeof(empty_pixels));
    pixel_count += empty_pixels;
    pic->read((char *)&pixel_num, sizeof(pixel_num));
#ifdef DEBUG
      cout << "Reading empty pixels: " << empty_pixels << endl;
      cout << "Reading pixel num: " << pixel_num << endl;
#endif

    byte_count += 4;
    empty_pixels = pixel_count + pixel_num;

    while (pixel_count < empty_pixels) {
      unsigned char red, green, blue, alpha;

      pic->read((char *)&red, sizeof(red));
      pic->read((char *)&green, sizeof(green));
      pic->read((char *)&blue, sizeof(blue));
      pic->read((char *)&alpha, sizeof(alpha));

      sprite->pixel[pixel_count / 32][pixel_count % 32].red = red;
      sprite->pixel[pixel_count / 32][pixel_count % 32].green = green;
      sprite->pixel[pixel_count / 32][pixel_count % 32].blue = blue;
      sprite->pixel[pixel_count / 32][pixel_count % 32].alpha = alpha;
#ifdef DEBUG
      // cout << "Reading RGB: " << red << green << blue << alpha << endl;
#endif

      // sprite->pixel[0][pixel_count].red = red;
      // sprite->pixel[0][pixel_count].green = green;
      // sprite->pixel[0][pixel_count].blue = blue;
      // sprite->pixel[0][pixel_count].alpha = false;

      byte_count += 4;
      pixel_count++;
    }
  }

  // Restore file add aress
  pic->seekg(adr);

  return 0;
}

int pic_extract(ifstream *pic) {
  unsigned version;
  unsigned short image_num;
  unsigned sprite_adr;

  ofstream data;

  timage *image;
  tsprite **sprite;
  tpixel **pixel;

  // Create a new directory.
  mkdir("images", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  // Create the pic.data file
  data.open("images/pic.data", ios::binary);
  if (!data.is_open()) {
    cerr << "Unable to open images/pic.data" << endl;
    exit(1);
  }

  // Read the Tibia.pic version and store it.
  pic->read((char *)&version, sizeof(version));
  data.write((char *)&version, sizeof(version));

  // Read how many images are stored.
  pic->read((char *)&image_num, sizeof(image_num));
  data.write((char *)&image_num, sizeof(image_num));

#ifdef DEBUG
  cout << "Version  : " << hex << version << endl;
  cout << "Image_num: " << image_num << endl;
#endif

  try {
    image = new timage[image_num];
    for (int a = 0; a < image_num; a++) {
      // Read how many sprites this image has.
      pic->read((char *)&(image[a].width), sizeof(image[0].width));
      pic->read((char *)&(image[a].height), sizeof(image[0].height));
#ifdef DEBUG
      cout << "Image N. " << a << endl;
      cout << "Width: " << (char **)image[a].width << endl;
      cout << "Height: " << (char **)image[a].height << endl;
#endif

      // I have no ideia what this next 3 bytes are for... so I will just save
      // them for later use
      pic->read((char *)&(image[a].unk0), sizeof(image[0].unk0));
      pic->read((char *)&(image[a].unk1), sizeof(image[0].unk1));
      pic->read((char *)&(image[a].unk2), sizeof(image[0].unk2));
      data.write((char *)&(image[a].unk0), sizeof(image[0].unk0));
      data.write((char *)&(image[a].unk1), sizeof(image[0].unk1));
      data.write((char *)&(image[a].unk2), sizeof(image[0].unk2));
#ifdef DEBUG
      cout << "Read empty bytes. " << endl;
#endif

      image[a].sprite = new tsprite *[image[a].height];
      for (int b = 0; b < image[a].height; b++) {
        image[a].sprite[b] = new tsprite[image[a].width];
        for (int c = 0; c < image[a].width; c++) {
          // Now we read the address where the sprite is stored.
          pic->read((char *)&sprite_adr, sizeof(sprite_adr));
#ifdef DEBUG
      cout << "Read image address. " << sprite_adr << endl;
#endif

          image[a].sprite[b][c].pixel = new tpixel *[32];
          for (int d = 0; d < 32; d++) {
            image[a].sprite[b][c].pixel[d] = new tpixel[32];

            // I want a full alpha sprite
            for (int e = 0; e < 32; e++) {
              image[a].sprite[b][c].pixel[d][e].red = 0;
              image[a].sprite[b][c].pixel[d][e].green = 0;
              image[a].sprite[b][c].pixel[d][e].blue = 0;
              image[a].sprite[b][c].pixel[d][e].alpha = 0;
            }
          }

          read_sprite(pic, sprite_adr, &image[a].sprite[b][c]);
        }
      }

      FILE *pngfile;
      ostringstream Stm;

      Stm << "images/" << setfill('0') << setw(3) << a << ".png";

      pngfile = fopen(Stm.str().c_str(), "wb");
      if (pngfile == NULL) {
        cerr << "Unable to open "
             << ".png file" << endl;
        exit(1);
      }

      write_png_image(pngfile, &image[a]);

      fclose(pngfile);
    }
  } catch (bad_alloc &) {
    cerr << "Error allocating memory." << endl;
    exit(1);
  }

  for (int a = 0; a < image_num; a++) {
    for (int b = 0; b < image[a].height; b++) {
      for (int c = 0; c < image[a].width; c++) {
        for (int d = 0; d < 32; d++) {
          delete image[a].sprite[b][c].pixel[d];
        }
        delete[] image[a].sprite[b][c].pixel;
      }
      delete[] image[a].sprite[b];
    }
    delete[] image[a].sprite;
  }
  delete[] image;
}

///////////////////////
//// READ PNG FILE ////
///////////////////////
int read_png_file(const char *file_name, timage *image) {
  png_bytep *row_pointers;

  png_byte header[8]; // 8 is the maximum size that can be checked

  // open file and test for it being a png
  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    // abort_("[read_png_file] File %s could not be opened for reading",
    // file_name);
    return 1;
  }

  fread(header, 1, 8, fp);

  if (png_sig_cmp(header, 0, 8)) {
    // abort_("[read_png_file] File %s is not recognized as a PNG file",
    // file_name);
    return 1;
  }

  // initialize stuff.
  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr) {
    // abort_("[read_png_file] png_create_read_struct failed");
    return 1;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    // abort_("[read_png_file] png_create_info_struct failed");
    exit(1);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    // abort_("[read_png_file] Error during init_io");
    exit(1);
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  image->width = png_get_image_width(png_ptr, info_ptr) / 32;
  image->height = png_get_image_height(png_ptr, info_ptr) / 32;
  // color_type = info_ptr->color_type;
  // bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  // number_of_passes = png_set_interlace_handling(png_ptr);

  if (png_get_color_type(png_ptr, info_ptr) != 6) {
    cerr << "Wrong color type!" << endl;
    exit(1);
  }

  if (png_get_bit_depth(png_ptr, info_ptr) != 8) {
    cerr << "Wrong bit depth!" << endl;
    exit(1);
  }
#ifdef DEBUG
  cout << "Color Type: " << (int)(png_get_color_type(png_ptr, info_ptr)) << endl;
  cout << "Bit Depth: " << (int)(png_get_bit_depth(png_ptr, info_ptr)) << endl;
#endif

  if (png_get_image_width(png_ptr, info_ptr) % 32) {
    cout << "ERROR: Invalid image width." << endl;
    exit(1);
  }

  if (png_get_image_height(png_ptr, info_ptr) % 32) {
    cout << "ERROR: Invalid image height." << endl;
    exit(1);
  }

#ifdef DEBUG
  cout << "Width: " << png_get_image_width(png_ptr, info_ptr)
       << "  Height: " << png_get_image_height(png_ptr, info_ptr) << endl;
#endif

  png_read_update_info(png_ptr, info_ptr);

  // read file.
  if (setjmp(png_jmpbuf(png_ptr))) {
    cout << "[read_png_file] Error during read_image" << endl;
    exit(1);
  }

  // Fill row_pointers
  row_pointers = new png_bytep[png_get_image_height(png_ptr, info_ptr)];
  for (int y = 0; y < png_get_image_height(png_ptr, info_ptr); y++)
    row_pointers[y] = new png_byte[png_get_rowbytes(png_ptr, info_ptr)];

  png_read_image(png_ptr, row_pointers);

  image->sprite = new tsprite *[image->height];
  for (int b = 0; b < image->height; b++) {
    image->sprite[b] = new tsprite[image->width];
    for (int c = 0; c < image->width; c++) {
      image->sprite[b][c].pixel = new tpixel *[32];
      for (int d = 0; d < 32; d++) {
        image->sprite[b][c].pixel[d] = new tpixel[32];
        /*
                // I want a full alpha sprite
                for( int e=0; e<32; e++ )
                {
                  image->sprite[b][c].pixel[d][e].red = 0;
                  image->sprite[b][c].pixel[d][e].green = 0;
                  image->sprite[b][c].pixel[d][e].blue = 0;
                  image->sprite[b][c].pixel[d][e].alpha = true;
                }
        */
      }
    }
  }

  for (int y = 0; y < png_get_image_height(png_ptr, info_ptr); y++) {
    png_byte *row = row_pointers[y];
    for (int x = 0; x < png_get_image_width(png_ptr, info_ptr); x++) {
      png_byte *ptr = &(row[x * 4]);

      image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].red = ptr[0];
      image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].green = ptr[1];
      image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].blue = ptr[2];
      image->sprite[y / 32][x / 32].pixel[y % 32][x % 32].alpha = ptr[3];
      // image->sprite[y/32][x/32].pixel[y%32][x%32].alpha = false;
    }
  }

  /*
  for( int i=0; i<5; i++ )
  {
    cout << " R: " << (int)image->sprite[0][0].pixel[0][i].red;
    cout << " G: " << (int)image->sprite[0][0].pixel[0][i].green;
    cout << " B: " << (int)image->sprite[0][0].pixel[0][i].blue << endl;;
  }
  exit(1);
*/

  // cleanup heap allocation
  for (int y = 0; y < image->height; y++)
    // free(row_pointers[y]);
    delete[] row_pointers[y];
  // free(row_pointers);
  delete[] row_pointers;

  fclose(fp);

  return 0;
}

////////////////////////
//// WRITE SPRITE  /////
////////////////////////
int write_sprite(ofstream *pic, unsigned int adress, tsprite *sprite) {
  // AdrBackup: integer;
  // SprCount, Count: Integer;
  // FileAdr: integer;
  // LeAux: Byte;
  int adr;
  int spr_count;
  int backup_adr;
  unsigned short int i = 0;

#ifdef DEBUG
  cout << "write_sprite" << endl;
#endif

  // Pop file adress
  adr = pic->tellp(); // AdrBackup := FilePos(Arq);

  pic->seekp(adress + 2); // Seek( Arq, Adress+2 );

  spr_count = 0; // SprCount := 0;
  while (spr_count < 1024) {
    i = 0;
    while ((spr_count < 1024) &&
           (sprite->pixel[spr_count / 32][spr_count % 32].alpha != 0xFF)) {
      spr_count++; // SprCount := SprCount + 1;
      i++;         // Count := Count + 1;
    }

#ifdef DEBUG
    cout << "1" << endl;
#endif

    pic->write((char *)&i, sizeof(i)); // WriteNBytes(2,Count);

    backup_adr = pic->tellp();
    pic->seekp(2, ios::cur); // Jump 2 bytes

#ifdef DEBUG
    cout << "1.5" << endl;
#endif

    i = 0;
    while ((spr_count < 1024) &&
           (sprite->pixel[spr_count / 32][spr_count % 32].alpha != 0x00)) {
      pic->write((char *)&(sprite->pixel[spr_count / 32][spr_count % 32].red),
                 sizeof(sprite->pixel[spr_count / 32][spr_count % 32].red));
      pic->write((char *)&(sprite->pixel[spr_count / 32][spr_count % 32].green),
                 sizeof(sprite->pixel[spr_count / 32][spr_count % 32].green));
      pic->write((char *)&(sprite->pixel[spr_count / 32][spr_count % 32].blue),
                 sizeof(sprite->pixel[spr_count / 32][spr_count % 32].blue));
      pic->write((char *)&(sprite->pixel[spr_count / 32][spr_count % 32].alpha),
                 sizeof(sprite->pixel[spr_count / 32][spr_count % 32].alpha));

      i++;
      spr_count++;
    }

#ifdef DEBUG
    cout << "2" << endl;
#endif

    pic->seekp(backup_adr);
    pic->write((char *)&i, sizeof(i));

    backup_adr = pic->tellp();
    backup_adr += i * 4;
    pic->seekp(i * 4, ios::cur);
  }

  pic->seekp(adress);

  unsigned short int aux;
  aux = backup_adr - adress - 2;
  pic->write((char *)&aux, sizeof(aux));

#ifdef DEBUG
  cout << "3" << endl;
#endif

  // Push File Adress
  pic->seekp(adr);

  return (backup_adr);
}

///////////////////////
//// PIC COMPILE  /////
///////////////////////
int pic_compile(ofstream *pic) {
  timage *image;

  ifstream png_image, data;

  unsigned int version;
  unsigned short int image_num;

  unsigned int sprite_begin = 0;

  // Open the pic.data file.
  data.open("images/pic.data", ios::binary);
  if (!data.is_open()) {
    cerr << "Unable to open images/pic.data" << endl;
    exit(1);
  }

  // Read the version and image_num.
  data.read((char *)&version, sizeof(version));
  data.read((char *)&image_num, sizeof(image_num));

  // Write the version and image_num.
  pic->write((char *)&version, sizeof(version));
  pic->write((char *)&image_num, sizeof(image_num));

  sprite_begin = image_num * 5 + sizeof(image_num) + sizeof(version);

  image = new timage[image_num];

  for (int i = 0; i < image_num; i++) {
    ostringstream Stm;

    Stm << "images/" << setfill('0') << setw(3) << i << ".png";

    png_image.open(Stm.str().c_str(), ios::binary);
    if (!png_image.is_open()) {
      cerr << "Unable to open " << Stm.str() << endl;
      return (0);
    }

    cout << "Opened " << Stm.str() << endl;

    read_png_file(Stm.str().c_str(), &image[i]);

    sprite_begin += image[i].width * image[i].height * 4;

    png_image.close();
  }

#ifdef DEBUG
  cout << "Sprite_begin: " << sprite_begin << endl;
#endif

  for (int i = 0; i < image_num; i++) {
    unsigned char aux1, aux2, aux3;

    pic->write((char *)&image[i].width, sizeof(image[i].width));
    pic->write((char *)&image[i].height, sizeof(image[i].height));

    data.read((char *)&aux1, sizeof(aux1));
    data.read((char *)&aux2, sizeof(aux2));
    data.read((char *)&aux3, sizeof(aux3));

    pic->write((char *)&aux1, sizeof(aux1));
    pic->write((char *)&aux2, sizeof(aux2));
    pic->write((char *)&aux3, sizeof(aux3));

    for (int a = 0; a < image[i].height; a++) {
      for (int b = 0; b < image[i].width; b++) {
        pic->write((char *)&sprite_begin, sizeof(sprite_begin));
        sprite_begin = write_sprite(pic, sprite_begin, &image[i].sprite[a][b]);
      }
    }
  }

  for (int a = 0; a < image_num; a++) {
    for (int b = 0; b < image[a].height; b++) {
      for (int c = 0; c < image[a].width; c++) {
        for (int d = 0; d < 32; d++) {
          delete image[a].sprite[b][c].pixel[d];
        }
        delete[] image[a].sprite[b][c].pixel;
      }
      delete[] image[a].sprite[b];
    }
    delete[] image[a].sprite;
  }
  delete[] image;

  return 0;
}

int main(int argc, char **argv) {
  int c;
  bool compile = false, extract = false;
  char *filename;
  ifstream pic_in;
  ofstream pic_out;

  while ((c = getopt(argc, argv, "c:x:")) != -1) {
    switch (c) {
    case 'c':
      compile = true;
      filename = optarg;
      break;
    case 'x':
      extract = true;
      filename = optarg;
      break;
    case '?':
      exit(1);
      break;
    default:
      exit(1);
      break;
    }
  }

  for (int index = optind; index < argc; index++) {
    cerr << "Non-option argument: " << argv[index] << endl;
    exit(1);
  }

  if ((compile && extract) || (!compile && !extract)) {
    cerr << "Usage: picparser [c|x] [file]" << endl;
    exit(1);
  }

  if (extract) {
    pic_in.open(filename, ios::binary);
    if (pic_in.is_open()) {
      pic_extract(&pic_in);
      pic_in.close();
    } else {
      cerr << "Unable to open " << filename << endl;
      exit(1);
    }
  }

  if (compile) {
    pic_out.open(filename, ios::out | ios::binary);
    if (pic_out.is_open()) {
      pic_compile(&pic_out);
      pic_out.close();
    } else {
      cerr << "Unable to open " << filename << endl;
      exit(1);
    }
  }

  // write_png();
  return 0;
}
