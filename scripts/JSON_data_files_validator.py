import os
import sys

try:
    project_path = f"{os.sep}".join(os.path.abspath(__file__).split(os.sep)[:-1])
    sys.path.append(project_path)
except Exception as e:
    print(f"Can not add project path to system path! Exiting!\nERROR: {e}")
    raise SystemExit(1)

import argparse
from collections import Counter
from collections.abc import Iterable
import json
import logging

import brotli
from schema import And, Optional, Schema
# Import VT related schemas
import LBDatafile_schema

def exc_handler(exception_type, exception, traceback):
    """ Exception handler for hiding traceback. """
    module_name = f"[{os.path.splitext(os.path.split(__file__)[-1])[0]}]"
    print(f"{module_name} {exception_type.__name__} {exception}")


class SchemaValidator:
    """ Validates schema of VT Object Map files (json)
    """
    def __init__(self, schema_type: str):
        self.schema_type = schema_type
        self.valid_schema = self._get_valid_schema()

    @staticmethod
    def get_error_message(iterable_collection: Iterable) -> str:
        """ Return error message. """
        return " or ".join(iterable_collection)

    def _get_valid_schema(self) -> Schema:
        """ Returns representation of a valid schema
        """
        valid_schema_data = LBDatafile_schema.LBDatafile_schema
        allowed_types_stats = ("LBStatsfile")
        valid_schema_stats = Schema(
            {
                Optional('type'): And(str, lambda a: a in allowed_types_stats,
                                      error=f"{self.get_error_message(allowed_types_stats)} must be chosen"),
                Optional('metadata'): {
                    Optional('type'): And(str, lambda a: a in allowed_types_stats,
                                          error=f"{self.get_error_message(allowed_types_stats)} must be chosen"),
                },
                'phases': [
                    {
                        "id": int,
                        Optional("migration count"): int,
                        Optional("post-LB"): {
                            "Object_comm": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Object_load_modeled": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Object_load_raw": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            Optional("Object_strategy_specific_load_modeled"): {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_comm": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_load_modeled": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_load_raw": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            Optional("Rank_strategy_specific_load_modeled"): {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            }
                        },
                        "pre-LB": {
                            "Object_comm": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Object_load_modeled": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Object_load_raw": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            Optional("Object_strategy_specific_load_modeled"): {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_comm": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_load_modeled": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            "Rank_load_raw": {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            },
                            Optional("Rank_strategy_specific_load_modeled"): {
                                "avg": float,
                                "car": float,
                                "imb": float,
                                "kur": float,
                                "max": float,
                                "min": float,
                                "npr": float,
                                "skw": float,
                                "std": float,
                                "sum": float,
                                "var": float
                            }
                        }
                    },
                ]
            }
        )

        if self.schema_type == "LBDatafile":
            return valid_schema_data
        elif self.schema_type == "LBStatsfile":
            return valid_schema_stats

        sys.excepthook = exc_handler
        raise TypeError(f"Unsupported schema type: {self.schema_type} was given")

    def is_valid(self, schema_to_validate: dict) -> bool:
        """ Returns True if schema_to_validate is valid with self.valid_schema else False. """
        is_valid = self.valid_schema.is_valid(schema_to_validate)
        return is_valid

    def validate(self, schema_to_validate: dict):
        """ Return validated schema. """
        sys.excepthook = exc_handler
        return self.valid_schema.validate(schema_to_validate)


class JSONDataFilesValidator:
    """ Class validating VT data files according do defined schema. """
    def __init__(self, file_path: str = None, dir_path: str = None,
                 file_prefix: str = None, file_suffix: str = None,
                 validate_comm_links: bool = False):
        self.__file_path = file_path
        self.__dir_path = dir_path
        self.__file_prefix = file_prefix
        self.__file_suffix = file_suffix
        self.__validate_comm_links = validate_comm_links
        self.__cli()

    def __cli(self):
        """ Support for common line arguments. """
        parser = argparse.ArgumentParser()
        group = parser.add_mutually_exclusive_group()
        group.add_argument("--dir_path", help="Path to directory where files for validation are located.")
        group.add_argument("--file_path", help="Path to a validated file. Pass only when validating a single file.")
        parser.add_argument("--file_prefix", help="File prefix. Optional. Pass only when --dir_path is provided.")
        parser.add_argument("--file_suffix", help="File suffix. Optional. Pass only when --dir_path is provided.")
        parser.add_argument("--validate_comm_links", help='Verify that comm links reference tasks.', action='store_true')
        args = parser.parse_args()
        if args.file_path:
            self.__file_path = os.path.abspath(args.file_path)
        if args.dir_path:
            self.__dir_path = os.path.abspath(args.dir_path)
        if args.file_prefix:
            self.__file_prefix = args.file_prefix
        if args.file_suffix:
            self.__file_suffix = args.file_suffix
        self.__validate_comm_links = args.validate_comm_links

    @staticmethod
    def __check_if_file_exists(file_path: str) -> bool:
        """ Check for existence of a given file. Returns True when file exists. """
        return os.path.isfile(file_path)

    @staticmethod
    def __check_if_dir_exists(dir_path: str) -> bool:
        """ Check for existence of a given directory. Returns True when file exists. """
        return os.path.isdir(dir_path)

    @staticmethod
    def __get_files_for_validation(dir_path: str, file_prefix: str, file_suffix: str) -> list:
        """ Get a sorted list of files from directory. """
        list_of_files = os.listdir(dir_path)

        if not list_of_files:
            sys.excepthook = exc_handler
            raise FileNotFoundError(f"Directory: {dir_path} is EMPTY")

        if file_prefix is None and file_suffix is None:
            print("File prefix and file suffix not given")
            file_prefix = Counter([file.split('.')[0] for file in list_of_files]).most_common()[0][0]
            print(f"Found most common prefix: {file_prefix}")
            file_suffix = Counter([file.split('.')[-1] for file in list_of_files]).most_common()[0][0]
            print(f"Found most common suffix: {file_suffix}")

        if file_prefix is not None:
            list_of_files = [file for file in list_of_files if file.split('.')[0] == file_prefix]

        if file_suffix is not None:
            list_of_files = [file for file in list_of_files if file.split('.')[-1] == file_suffix]

        return sorted([os.path.join(dir_path, file) for file in list_of_files],
                      key=lambda x: int(x.split(os.sep)[-1].split('.')[-2]))

    @staticmethod
    def __validate_file(file_path, validate_comm_links):
        """ Validates the file against the schema. """
        logging.info(f"Validating file: {file_path}")
        with open(file_path, "rb") as json_file:
            content = json_file.read()
            if file_path.endswith('.br'):
                content = brotli.decompress(content)
            json_data = json.loads(content.decode("utf-8"))

        # Extracting type from JSON data
        schema_type = None
        if json_data.get("metadata") is not None:
            schema_type = json_data.get("metadata").get("type")
        else:
            if json_data.get("type") is not None:
                schema_type = json_data.get("type")

        if schema_type is not None:
            # Validate schema
            if SchemaValidator(schema_type=schema_type).is_valid(schema_to_validate=json_data):
                logging.info(f"Valid JSON schema in {file_path}")
            else:
                logging.error(f"Invalid JSON schema in {file_path}")
                SchemaValidator(schema_type=schema_type).validate(schema_to_validate=json_data)
        else:
            logging.warning(f"Schema type not found in file: {file_path}. \nPassing by default when schema type not found.")

        if validate_comm_links:
            logging.info("FIXME: comm_links validation not implemented.")


    def main(self):
        if self.__file_path is not None:
            if self.__check_if_file_exists(file_path=self.__file_path):
                self.__validate_file(file_path=self.__file_path,
                                     validate_comm_links=self.__validate_comm_links)
            else:
                sys.excepthook = exc_handler
                raise FileNotFoundError(f"File: {self.__file_path} NOT found")
        elif self.__dir_path is not None:
            if self.__check_if_dir_exists(dir_path=self.__dir_path):
                list_of_files_for_validation = self.__get_files_for_validation(dir_path=self.__dir_path,
                                                                               file_prefix=self.__file_prefix,
                                                                               file_suffix=self.__file_suffix)
                for file in list_of_files_for_validation:
                    self.__validate_file(file_path=file,
                                         validate_comm_links=self.__validate_comm_links)
            else:
                sys.excepthook = exc_handler
                raise FileNotFoundError(f"Directory: {self.__dir_path} does NOT exist")
        else:
            sys.excepthook = exc_handler
            raise Exception("FILE path or DIRECTORY path has to be given")


if __name__ == "__main__":
    JSONDataFilesValidator().main()
