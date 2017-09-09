#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaml.h>

#include "config.h"
#include "utils.h"

typedef struct
{
   const char *name;
   const section_type section;
} section_entry;

typedef struct
{
   const char *name;
   const texture_format format;
} format_entry;

static const section_entry section_table[] = {
   {"asm",      TYPE_ASM},
   {"behavior", TYPE_BEHAVIOR},
   {"bin",      TYPE_BIN},
   {"blast",    TYPE_BLAST},
   {"geo",      TYPE_GEO},
   {"gzip",     TYPE_GZIP},
   {"header",   TYPE_HEADER},
   {"instrset", TYPE_INSTRUMENT_SET},
   {"level",    TYPE_LEVEL},
   {"m64",      TYPE_M64},
   {"mio0",     TYPE_MIO0},
   {"ptr",      TYPE_PTR},
};

static const format_entry format_table[] = {
   {"rgba",      FORMAT_RGBA},
   {"ia",        FORMAT_IA},
   {"i",         FORMAT_I},
   {"skybox",    FORMAT_SKYBOX},
   {"collision", FORMAT_COLLISION},
};

static section_type str2section(const char *type_name)
{
   unsigned i;
   if (type_name != NULL) {
      for (i = 0; i < DIM(section_table); i++) {
         if (0 == strcmp(section_table[i].name, type_name)) {
            return section_table[i].section;
         }
      }
   }
   return TYPE_INVALID;
}

static texture_format str2format(const char *format_name)
{
   unsigned i;
   if (format_name != NULL) {
      for (i = 0; i < DIM(format_table); i++) {
         if (0 == strcmp(format_table[i].name, format_name)) {
            return format_table[i].format;
         }
      }
   }
   return FORMAT_INVALID;
}

int get_scalar_value(char *scalar, yaml_node_t *node)
{
   if (node->type == YAML_SCALAR_NODE) {
      memcpy(scalar, node->data.scalar.value, node->data.scalar.length);
      scalar[node->data.scalar.length] = '\0';
      return node->data.scalar.length;
   } else {
      return -1;
   }
}

int get_scalar_uint(unsigned int *val, yaml_node_t *node)
{
   char value[32];
   int status = get_scalar_value(value, node);
   if (status > 0) {
      *val = strtoul(value, NULL, 0);
   }
   return status;
}

void load_memory(unsigned int *mem, yaml_document_t *doc, yaml_node_t *node)
{
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   if (node->type == YAML_SEQUENCE_NODE) {
      size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
      if (count == 3) {
         i_node = node->data.sequence.items.start;
         for (size_t i = 0; i < count; i++) {
            next_node = yaml_document_get_node(doc, i_node[i]);
            if (next_node && next_node->type == YAML_SCALAR_NODE) {
               get_scalar_uint(&mem[i], next_node);
            } else {
               ERROR("Error: non-scalar value in memory sequence\n");
            }
         }
      } else {
         ERROR("Error: memory sequence needs 3 scalars\n");
      }
   }
}

int load_memory_sequence(rom_config *c, yaml_document_t *doc, yaml_node_t *node)
{
   yaml_node_t *next_node;
   int ret_val = -1;
   if (node->type == YAML_SEQUENCE_NODE) {
      size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
      c->ram_table = malloc(3 * count * sizeof(*c->ram_table));
      c->ram_count = 0;
      yaml_node_item_t *i_node = node->data.sequence.items.start;
      for (size_t i = 0; i < count; i++) {
         next_node = yaml_document_get_node(doc, i_node[i]);
         if (next_node && next_node->type == YAML_SEQUENCE_NODE) {
            load_memory(&c->ram_table[3*c->ram_count], doc, next_node);
            c->ram_count++;
         } else {
            ERROR("Error: non-sequence in memory sequence\n");
         }
      }
      ret_val = 0;
   }
   return ret_val;
}

void load_texture(texture *tex, yaml_document_t *doc, yaml_node_t *node)
{
   char val[64];
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
   i_node = node->data.sequence.items.start;
   for (size_t i = 0; i < count; i++) {
      next_node = yaml_document_get_node(doc, i_node[i]);
      if (next_node && next_node->type == YAML_SCALAR_NODE) {
         get_scalar_value(val, next_node);
         switch (i) {
            case 0: tex->offset = strtoul(val, NULL, 0); break;
            case 1:
               tex->format = str2format(val);
               if (tex->format == FORMAT_INVALID) {
                  ERROR("Error: %ld - invalid texture format '%s'\n", node->start_mark.line, val);
                  return;
               }
               break;
            case 2: tex->depth = strtoul(val, NULL, 0); break;
            case 3: tex->width = strtoul(val, NULL, 0); break;
            case 4: tex->height = strtoul(val, NULL, 0); break;
         }
      } else {
         ERROR("Error: non-scalar value in texture sequence\n");
      }
   }
}

void load_behavior(behavior *beh, yaml_document_t *doc, yaml_node_t *node)
{
   char val[64];
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
   i_node =  node->data.sequence.items.start;
   if (count != 2) {
      ERROR("Error: %ld - expected 2 fields for behavior, got %ld\n", node->start_mark.line, count);
      return;
   }
   for (size_t i = 0; i < count; i++) {
      next_node = yaml_document_get_node(doc, i_node[i]);
      if (next_node && next_node->type == YAML_SCALAR_NODE) {
         get_scalar_value(val, next_node);
         switch (i) {
            case 0: beh->offset = strtoul(val, NULL, 0); break;
            case 1: strcpy(beh->name, val); break;
         }
      } else {
         ERROR("Error: non-scalar value in behavior sequence\n");
      }
   }
}

void load_section_data(split_section *section, yaml_document_t *doc, yaml_node_t *node)
{
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
   i_node = node->data.sequence.items.start;
   switch (section->type) {
      case TYPE_BLAST:
      case TYPE_MIO0:
      case TYPE_GZIP:
      {
         // parse texture
         texture *tex = malloc(count * sizeof(*tex));
         for (size_t i = 0; i < count; i++) {
            next_node = yaml_document_get_node(doc, i_node[i]);
            if (next_node->type == YAML_SEQUENCE_NODE) {
               load_texture(&tex[i], doc, next_node);
            } else {
               ERROR("Error: non-sequence texture node\n");
               return;
            }
         }
         section->extra = tex;
         section->extra_len = count;
         break;
      }
      case TYPE_BEHAVIOR:
      {
         // parse behavior
         behavior *beh = malloc(count * sizeof(*beh));
         for (size_t i = 0; i < count; i++) {
            next_node = yaml_document_get_node(doc, i_node[i]);
            if (next_node->type == YAML_SEQUENCE_NODE) {
               load_behavior(&beh[i], doc, next_node);
            } else {
               ERROR("Error: non-sequence texture node\n");
               return;
            }
         }
         section->extra = beh;
         section->extra_len = count;
         break;
      }
      default:
         ERROR("Warning: %ld - extra fields for section\n", node->start_mark.line);
         break;
   }
}

void load_section(split_section *section, yaml_document_t *doc, yaml_node_t *node)
{
   char val[128];
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
   if (count >= 3) {
      i_node = node->data.sequence.items.start;
      for (int i = 0; i < 3; i++) {
         next_node = yaml_document_get_node(doc, i_node[i]);
         if (next_node && next_node->type == YAML_SCALAR_NODE) {
            get_scalar_value(val, next_node);
            switch (i) {
               case 0: section->start = strtoul(val, NULL, 0); break;
               case 1: section->end = strtoul(val, NULL, 0); break;
               case 2: section->type = str2section(val); break;
            }
         } else {
            ERROR("Error: non-scalar value in section sequence\n");
         }
      }
      section->extra = NULL;
      section->extra_len = 0;
      // validate section parameter counts
      switch (section->type) {
         case TYPE_ASM:
         case TYPE_BIN:
         case TYPE_HEADER:
         case TYPE_GEO:
         case TYPE_LEVEL:
         case TYPE_M64:
            if (count > 4) {
               ERROR("Error: %ld - expected 3-4 fields for section\n", node->start_mark.line);
               return;
            }
            break;
         case TYPE_INSTRUMENT_SET:
         case TYPE_PTR:
            if (count > 5) {
               ERROR("Error: %ld - expected 3-5 fields for section\n", node->start_mark.line);
               return;
            }
            break;
         case TYPE_BEHAVIOR:
         case TYPE_BLAST:
         case TYPE_MIO0:
         case TYPE_GZIP:
            if (count < 4 || count > 5) {
               ERROR("Error: %ld - expected 4-5 fields for section\n", node->start_mark.line);
               return;
            }
            break;
         default:
            ERROR("Error: %ld - invalid section type '%s'\n", node->start_mark.line, val);
            return;
      }
      if (count > 3) {
         next_node = yaml_document_get_node(doc, i_node[3]);
         if (next_node && next_node->type == YAML_SCALAR_NODE) {
            get_scalar_value(val, next_node);
            switch (section->type) {
               case TYPE_BLAST:
                  section->subtype = strtoul(val, NULL, 0);
                  break;
               default:
                  strcpy(section->label, val);
                  break;
            }
         } else {
            ERROR("Error: non-scalar value in section sequence\n");
         }
      } else {
         section->label[0] = '\0';
      }
      // extra parameters for some types
      if (count > 4) {
         next_node = yaml_document_get_node(doc, i_node[4]);
         if (next_node && next_node->type == YAML_SCALAR_NODE) {
            get_scalar_value(val, next_node);
            switch (section->type) {
               case TYPE_PTR:
               case TYPE_INSTRUMENT_SET:
                  section->extra_len = strtoul(val, NULL, 0);
                  break;
               default:
                  ERROR("Warning: %ld - extra fields for section\n", node->start_mark.line);
                  break;
            }
         } else {
            ERROR("Error: non-scalar value in section sequence\n");
         }
      }
   } else {
      ERROR("Error: section sequence needs 3-5 scalars (got %ld)\n", count);
   }
}

int load_sections_sequence(rom_config *c, yaml_document_t *doc, yaml_node_t *node)
{
   yaml_node_t *next_node;
   yaml_node_t *key_node;
   yaml_node_t *val_node;
   int ret_val = -1;
   if (node->type == YAML_SEQUENCE_NODE) {
      size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
      c->sections = malloc(count * sizeof(*c->sections));
      c->section_count = 0;
      yaml_node_item_t *i_node = node->data.sequence.items.start;
      for (size_t i = 0; i < count; i++) {
         next_node = yaml_document_get_node(doc, i_node[i]);
         if (next_node) {
            if (next_node->type == YAML_SEQUENCE_NODE) {
               load_section(&c->sections[c->section_count], doc, next_node);
               c->section_count++;
            } else if (next_node->type == YAML_MAPPING_NODE) {
               yaml_node_pair_t *i_node_p;
               for (i_node_p = next_node->data.mapping.pairs.start; i_node_p < next_node->data.mapping.pairs.top; i_node_p++) {
                  // the key node is a sequence
                  key_node = yaml_document_get_node(doc, i_node_p->key);
                  val_node = yaml_document_get_node(doc, i_node_p->value);
                  if (key_node && val_node && key_node->type == YAML_SEQUENCE_NODE && val_node->type == YAML_SEQUENCE_NODE) {
                     load_section(&c->sections[c->section_count], doc, key_node);
                     load_section_data(&c->sections[c->section_count], doc, val_node);
                     c->section_count++;
                  } else {
                     ERROR("ERROR: sections sequence map is not sequence\n");
                  }
               }
            } else {
               ERROR("Error: sections sequence contains non-sequence and non-mapping\n");
            }
         }
      }
      ret_val = 0;
   }
   return ret_val;
}

void load_label(label *lab, yaml_document_t *doc, yaml_node_t *node)
{
   char val[128];
   yaml_node_item_t *i_node;
   yaml_node_t *next_node;
   if (node->type == YAML_SEQUENCE_NODE) {
      size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
      if (count >= 2 && count <= 3) {
         i_node = node->data.sequence.items.start;
         for (size_t i = 0; i < count; i++) {
            next_node = yaml_document_get_node(doc, i_node[i]);
            if (next_node && next_node->type == YAML_SCALAR_NODE) {
               get_scalar_value(val, next_node);
               switch (i) {
                  case 0: lab->ram_addr = strtoul(val, NULL, 0); break;
                  case 1: strcpy(lab->name, val); break;
                  case 2: lab->end_addr = strtoul(val, NULL, 0); break;
               }
            } else {
               ERROR("Error: non-scalar value in label sequence\n");
            }
         }
      } else {
         ERROR("Error: label sequence needs 2-3 scalars\n");
      }
   }
}

int load_labels_sequence(rom_config *c, yaml_document_t *doc, yaml_node_t *node)
{
   yaml_node_t *next_node;
   int ret_val = -1;
   if (node->type == YAML_SEQUENCE_NODE) {
      size_t count = node->data.sequence.items.top - node->data.sequence.items.start;
      c->labels = malloc(count * sizeof(*c->labels));
      c->label_count = 0;
      yaml_node_item_t *i_node = node->data.sequence.items.start;
      for (size_t i = 0; i < count; i++) {
         next_node = yaml_document_get_node(doc, i_node[i]);
         if (next_node && next_node->type == YAML_SEQUENCE_NODE) {
            load_label(&c->labels[c->label_count], doc, next_node);
            c->label_count++;
         } else {
            ERROR("Error: non-sequence in labels sequence\n");
         }
      }
      ret_val = 0;
   }
   return ret_val;
}

void parse_yaml_root(yaml_document_t *doc, yaml_node_t *node, rom_config *c)
{
   char key[128];
   yaml_node_pair_t *i_node_p;

   yaml_node_t *key_node;
   yaml_node_t *val_node;

   // only accept mapping nodes at root level
   switch (node->type) {
      case YAML_MAPPING_NODE:
         for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
            key_node = yaml_document_get_node(doc, i_node_p->key);
            if (key_node) {
               get_scalar_value(key, key_node);
               val_node = yaml_document_get_node(doc, i_node_p->value);
               if (!strcmp(key, "name")) {
                  get_scalar_value(c->name, val_node);
                  INFO("config.name: %s\n", c->name);
               } else if (!strcmp(key, "basename")) {
                  get_scalar_value(c->basename, val_node);
                  INFO("config.basename: %s\n", c->name);
               } else if (!strcmp(key, "checksum1")) {
                  unsigned int val;
                  get_scalar_uint(&val, val_node);
                  c->checksum1 = (unsigned int)val;
               } else if (!strcmp(key, "checksum2")) {
                  unsigned int val;
                  get_scalar_uint(&val, val_node);
                  c->checksum2 = (unsigned int)val;
               } else if (!strcmp(key, "memory")) {
                  load_memory_sequence(c, doc, val_node);
               } else if (!strcmp(key, "ranges")) {
                  load_sections_sequence(c, doc, val_node);
               } else if (!strcmp(key, "labels")) {
                  load_labels_sequence(c, doc, val_node);
               }
            } else {
               ERROR("Couldn't find next node\n");
            }
         }
         break;
      default:
         ERROR("Error: %ld only mapping node supported at root level\n", node->start_mark.line);
         break;
   }
}

int parse_config_file(const char *filename, rom_config *c)
{
   yaml_parser_t parser;
   yaml_document_t doc;
   yaml_node_t *root;
   FILE *file;

   c->name[0] = '\0';
   c->basename[0] = '\0';
   c->ram_count = 0;
   c->section_count = 0;
   c->label_count = 0;

   // read config file, exit if problem
   file = fopen(filename, "rb");
   if (!file) {
      ERROR("Error: cannot open %s\n", filename);
      return -1;
   }
   yaml_parser_initialize(&parser);
   yaml_parser_set_input_file(&parser, file);

   if (!yaml_parser_load(&parser, &doc)) {
      ERROR("Error: problem parsing YAML %s:%ld %s (%d)\n", filename, parser.problem_offset, parser.problem, (int)parser.error);
   }

   root = yaml_document_get_root_node(&doc);
   if (root) {
      parse_yaml_root(&doc, root, c);
   }

   yaml_document_delete(&doc);
   yaml_parser_delete(&parser);

   fclose(file);

   return 0;
}

void print_config(const rom_config *config)
{
   int i, j;
   unsigned int *r = config->ram_table;
   split_section *s = config->sections;
   label *l = config->labels;

   printf("name: %s\n", config->name);
   printf("basename: %s\n", config->basename);
   printf("checksum1: %08X\n", config->checksum1);
   printf("checksum2: %08X\n\n", config->checksum2);

   // memory
   printf("memory:\n");
   printf("%-12s %-12s %-12s\n", "start", "end", "offset");
   for (i = 0; i < config->ram_count; i++) {
      printf("0x%08X   0x%08X   0x%08X\n", r[3*i], r[3*i+1], r[3*i+2]);
   }
   printf("\n");

   // ranges
   printf("ranges:\n");
   for (i = 0; i < config->section_count; i++) {
      printf("0x%08X 0x%08X %d %s %d\n", s[i].start, s[i].end, s[i].type, s[i].label, s[i].extra_len);
      if (s[i].extra_len > 0) {
         switch (s[i].type) {
            case TYPE_BLAST:
            case TYPE_MIO0:
            case TYPE_GZIP:
            {
               texture *tex = s[i].extra;
               for (j = 0; j < s[i].extra_len; j++) {
                  printf("  0x%06X %d", tex[j].offset, tex[j].format);
                  if (tex[j].format != FORMAT_COLLISION) {
                     printf(" %2d %3d %3d", tex[j].depth, tex[j].width, tex[j].height);
                  }
                  printf("\n");
               }
               break;
            }
            case TYPE_BEHAVIOR:
            {
               behavior *beh = s[i].extra;
               for (j = 0; j < s[i].extra_len; j++) {
                  printf("  0x%06X %s\n", beh[j].offset, beh[j].name);
               }
               break;
            }
            default:
               break;
         }
      }
   }
   printf("\n");

   // labels
   printf("labels:\n");
   for (i = 0; i < config->label_count; i++) {
      char end_str[16];
      if (l[i].end_addr) {
         sprintf(end_str, "%08X", l[i].end_addr);
      } else {
         sprintf(end_str, "        ");
      }
      printf("0x%08X-%s %s\n", l[i].ram_addr, end_str, l[i].name);
   }
}

int validate_config(const rom_config *config, unsigned int max_len)
{
   // error on overlapped and out-of-order sections
   int i, j, beh_i;
   unsigned int last_end = 0;
   int ret_val = 0;
   for (i = 0; i < config->section_count; i++) {
      split_section *isec = &config->sections[i];
      if (isec->start < last_end) {
         ERROR("Error: section %d \"%s\" (%X-%X) out of order\n",
               i, isec->label, isec->start, isec->end);
         ret_val = -2;
      }
      if (isec->end > max_len) {
         ERROR("Error: section %d \"%s\" (%X-%X) past end of file (%X)\n",
               i, isec->label, isec->start, isec->end, max_len);
         ret_val = -3;
      }
      if (isec->start >= isec->end) {
         ERROR("Error: section %d \"%s\" (%X-%X) invalid range\n",
               i, isec->label, isec->start, isec->end);
         ret_val = -4;
      }
      for (j = 0; j < i; j++) {
         split_section *jsec = &config->sections[j];
         if (!((isec->end <= jsec->start) || (isec->start >= jsec->end))) {
            ERROR("Error: section %d \"%s\" (%X-%X) overlaps %d \"%s\" (%X-%X)\n",
                  i, isec->label, isec->start, isec->end,
                  j, jsec->label, jsec->start, jsec->end);
            ret_val = -1;
         }
      }
      last_end = isec->end;
   }
   // error duplicate label addresses
   for (i = 0; i < config->label_count; i++) {
      for (j = i+1; j < config->label_count; j++) {
         if (config->labels[i].ram_addr == config->labels[j].ram_addr) {
            ERROR("Error: duplicate label %X \"%s\" \"%s\"\n", config->labels[i].ram_addr,
                  config->labels[i].name, config->labels[j].name);
            ret_val = -5;
         }
         if (0 == strcmp(config->labels[i].name, config->labels[j].name)) {
            ERROR("Error: duplicate label name \"%s\" %X %X\n", config->labels[i].name,
                  config->labels[i].ram_addr, config->labels[j].ram_addr);
            ret_val = -5;
         }
      }
   }
   // error duplicate behavior addresses
   beh_i = -1;
   for (i = 0; i < config->section_count; i++) {
      split_section *isec = &config->sections[i];
      if (isec->type == TYPE_BEHAVIOR) {
         beh_i = i;
         break;
      }
   }
   if (beh_i >= 0) {
      behavior *beh = config->sections[beh_i].extra;
      int extra_count = config->sections[beh_i].extra_len;
      for (i = 0; i < extra_count; i++) {
         for (j = i+1; j < extra_count; j++) {
            if (beh[i].offset == beh[j].offset) {
               ERROR("Error: duplicate behavior offset %04X \"%s\" \"%s\"\n", beh[i].offset,
                     beh[i].name, beh[j].name);
               ret_val = -6;
            }
            if (0 == strcmp(beh[i].name, beh[j].name)) {
               ERROR("Error: duplicate behavior name \"%s\" %04X %04X\n", beh[i].name,
                     beh[i].offset, beh[j].offset);
               ret_val = -6;
            }
         }
      }
   }
   return ret_val;
}

const char *config_get_version(void)
{
   static char version[16];
   int major, minor, patch;
   yaml_get_version(&major, &minor, &patch);
   sprintf(version, "%d.%d.%d", major, minor, patch);
   return version;
}

#ifdef YAML_CONFIG_TEST
int main(int argc, char *argv[])
{
   rom_config config;
   int status;

   if (argc < 2) {
      printf("Usage: yamlconfig <file.yaml>\n");
      return 1;
   }

   status = parse_config_file(argv[1], &config);
   if (status == 0) {
      print_config(&config);
   } else {
      ERROR("Error parsing %s: %d\n", argv[1], status);
   }

   return 0;
}
#endif