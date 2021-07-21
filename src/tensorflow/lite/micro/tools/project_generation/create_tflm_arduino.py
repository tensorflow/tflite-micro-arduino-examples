# Lint as: python3
# Copyright 2021 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Script to generate TFLM Arduino examples ZIP file"""

import argparse
import logging
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple, Union
from enum import Enum, unique
import configparser

# parse args: {outdir}
# generate list of all source/header files and their src/dst locations
# create directory tree in {outdir}/tflm_arduino
# download third-party source
# copy templates: library.properties to {outdir}/tflm_arduino
# copy templates: TensorFlowLite.h to {outdir}/tflm_arduino/src
# copy transforms: examples
#   --platform=arduino
#   --third_party_headers=<output from make
#       list_{example_name}_example_headers
#       list_third_party_headers>
#   --is_example_source
# copy transforms: examples/{example_name}/main_functions.cc to
#  {example_name}.ino
#   --platform=arduino
#   --third_party_headers=<output from make
#       list_{example_name}_example_headers
#       list_third_party_headers>
#   --is_example_ino
# copy transforms: tensorflow, third_party
#   --platform=arduino
#   --third_party_headers=<output from make
#       list_third_party_headers>
# patch third_party/flatbuffers/include/flatbuffers/base.h with:
#   sed -E 's/utility\.h/utility/g'
# run fix_arduino_subfolders.py {outdir}/tflm_arduino
# remove all empty directories in {outdir}/tflm_arduino tree
# create ZIP file using shutil.make_archive()


def RunSedScripts(file_path: Path,
                  scripts: List[str],
                  args: Union[str, None] = None,
                  is_dry_run: bool = True) -> None:
  """
  Run SED scripts with specified arguments against the given file.
  The file is updated in place.

  Args:
    file_path: The full path to the input file
    scripts: A list of strings, each containing a single SED script
    args: a string containing all the SED arguments | None
    is_dry_run: if True, do not execute any commands
  """
  cmd = "sed"
  if args is not None and args != "":
    cmd += f" {args}"
  for scr in scripts:
    cmd += f" -E {scr}"
  cmd += f" < {file_path!s}"
  print(f"Running command: {cmd}")
  if not is_dry_run:
    result = subprocess.run(cmd,
                            shell=True,
                            check=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    print(f"Saving output to: {file_path!s}")
    file_path.write_bytes(result.stdout)


def RemoveDirectories(paths: List[Path], is_dry_run: bool = True) -> None:
  """
  Remove directory tree(s) given list of pathnames

  Args:
    paths: A list of Path objects
    is_dry_run: if True, do not execute any commands
  """
  for dir_path in paths:
    print(f"Removing directory tree {dir_path!s}")
    if dir_path.exists() and not is_dry_run:
      shutil.rmtree(dir_path)


def RemoveEmptyDirectories(paths: Iterable[Path],
                           root: Path,
                           is_dry_run: bool = True) -> None:
  """
  Remove empty directories given list of pathnames, searching parent
  directories until reaching the root directory

  Args:
    paths: A list of Path objects
    root: The path at which to stop parent directory search
    is_dry_run: if True, do not execute any commands
  """
  empty_paths = list(filter(lambda p: list(p.glob("*")) == [], paths))
  parent_paths: Set[Path] = set()
  for dir_path in empty_paths:
    if dir_path == root:
      continue
    parent_paths.add(dir_path.parent)
    print(f"Removing empty directory {dir_path!s}")
    if not is_dry_run:
      dir_path.rmdir()
  if len(parent_paths) > 0:
    RemoveEmptyDirectories(parent_paths, root=root, is_dry_run=is_dry_run)


def RunPythonScript(path_to_script: Path,
                    args: str,
                    is_dry_run: bool = True) -> None:
  """
  Run a python script with specified arguments

  Args:
    path_to_script: The full path to the Python script
    args: a string containing all the script arguments
    is_dry_run: if True, do not execute any commands
  """
  cmd = f"python3 {path_to_script!s}"
  cmd += f" {args}"
  print(f"Running command: {cmd}")
  if not is_dry_run:
    _ = subprocess.run(cmd,
                       shell=True,
                       check=True,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)


def CreateDirectories(paths: List[Path], is_dry_run: bool = True) -> None:
  """
  Create directory tree(s) given list of pathnames

  Args:
    paths: A list of Path objects
    is_dry_run: if True, do not execute any commands
  """
  dir_path: Path
  for dir_path in paths:
    print(f"Creating directory tree {dir_path!s}")
    if not dir_path.is_dir() and not is_dry_run:
      dir_path.mkdir(mode=0o755, parents=True, exist_ok=True)


def CopyFiles(paths: Iterable[Tuple[Path, Path]],
              is_dry_run: bool = True) -> None:
  """
  Copy files given list of source and destination Path tuples

  Args:
    paths: A list of tuples of Path objects.
    Each tuple is of the form (source, destination)
    is_dry_run: if True, do not execute any commands
  """
  dir_path: Tuple[Path, Path]
  for dir_path in paths:
    print(f"Copying {dir_path[0]!s} to {dir_path[1]!s}")
    if not is_dry_run:
      shutil.copy2(dir_path[0], dir_path[1])


class ArduinoProjectGenerator:
  """
  Generate the TFLM Arduino library ZIP file
  """

  #
  # private enums
  #

  @unique
  class Manifest(Enum):
    ADD = "Add",
    REMOVE = "Remove",
    SPECIAL_REPO = "Special Repo",
    SPECIAL_BASE = "Special Base"
    PATCH_SED = "Patch Sed"

  #
  # private methods
  #

  def __init__(self) -> None:
    args = self._ParseArguments().parse_args()
    self.base_dir = Path(args.base_dir)
    if args.output_dir is None:
      self.output_dir = Path(tempfile.gettempdir()) / "tflm_arduino"
    else:
      self.output_dir = Path(args.output_dir)
    self.is_dry_run: bool = args.is_dry_run
    if args.manifest_file is None:
      self.manifest_path = Path(
          "src/tensorflow/lite/micro/tools/project_generation/MANIFEST.ini")
    else:
      self.manifest_path = Path(args.manifest_file)

    # generate list of examples by inspecting base_dir/examples
    self.examples: List[str] = []
    for path in self.base_dir.glob("examples/*"):
      if path.is_dir:
        self.examples.append(path.name)

    # parse manifest file
    manifest = self._ParseManifest()
    self.add_list: List[Path] = manifest[self.Manifest.ADD]
    self.remove_list: List[Path] = manifest[self.Manifest.REMOVE]
    self.special_repo_list: List[Tuple[Path, Path]] = manifest[
        self.Manifest.SPECIAL_REPO]
    self.special_base_list: List[Tuple[Path, Path]] = manifest[
        self.Manifest.SPECIAL_BASE]
    self.patch_sed_list: List[Tuple[List[Path], List[str]]] = manifest[
        self.Manifest.PATCH_SED]

    #self.downloads_path = Path("tensorflow/lite/micro/tools/make/downloads")

  def _ParseManifest(self) -> Dict[Manifest, List]:
    manifest: Dict[self.Manifest, List] = dict()
    parser = configparser.ConfigParser()
    _ = parser.read(self.manifest_path)
    files = filter(None, parser["Add Files"]["files"].splitlines())
    manifest[self.Manifest.ADD] = list(map(Path, files))
    files = filter(None, parser["Remove Files"]["files"].splitlines())
    manifest[self.Manifest.REMOVE] = list(map(Path, files))
    manifest[self.Manifest.SPECIAL_REPO] = []
    manifest[self.Manifest.SPECIAL_BASE] = []
    manifest[self.Manifest.PATCH_SED] = []
    for section in parser.sections():
      if section in ["Add Files", "Remove Files", "DEFAULT"]:
        continue
      # check if patch with sed scripts
      sed_scripts = parser[section].get("sed_scripts")
      if sed_scripts is not None:
        sed_scripts = filter(None, sed_scripts.splitlines())
        sed_scripts = list(sed_scripts)
        files = filter(None, parser[section]["files"].splitlines())
        files = map(lambda file: Path(file.strip()), files)
        files = list(files)
        manifest[self.Manifest.PATCH_SED].append((files, sed_scripts))
      else:
        to_file = parser[section]["to"]
        from_file = parser[section].get("from_repo")
        if from_file is None:
          from_file = parser[section]["from"]
          key = self.Manifest.SPECIAL_BASE
        else:
          key = self.Manifest.SPECIAL_REPO
        path_tuple = (Path(from_file.strip()), Path(to_file.strip()))
        manifest[key].append(path_tuple)

    return manifest

  def _ParseArguments(self) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Script for TFLM Arduino project generation")
    parser.add_argument("--output_dir",
                        type=str,
                        default=None,
                        help="Output directory for generated TFLM tree")
    parser.add_argument("--base_dir",
                        type=str,
                        required=True,
                        help="Base directory for generating TFLM tree")
    parser.add_argument("--manifest_file",
                        type=str,
                        default=None,
                        help="Manifest file path (relative or absolute)")
    parser.add_argument("--is_dry_run",
                        default=False,
                        action="store_true",
                        help="Show commands only (no execution)")
    return parser

  def _CleanOutputDirectory(self) -> None:
    dirs_to_remove = [self.output_dir]
    RemoveDirectories(dirs_to_remove, is_dry_run=self.is_dry_run)
    zip_path = self.output_dir.with_suffix(".zip")
    print(f"Removing ZIP file: {zip_path!s}")
    if zip_path.exists() and not self.is_dry_run:
      zip_path.unlink()

  def _CreateOutputDirectories(self, all_path_pairs: List[Tuple[Path,
                                                                Path]]) -> None:
    # generate full list of source tree directories
    # collect relative destination paths and sort relative paths
    set_relative_subdirs: Set[Path] = {
        path[1].parent for path in all_path_pairs if path[1].parent != Path(".")
    }
    relative_subdirs = list(set_relative_subdirs)
    relative_subdirs.sort()

    # filter out common parents
    def _FilterFunc(pair: Tuple[int, Path]):
      index = pair[0]
      if index == len(relative_subdirs) - 1:
        return True
      elif pair[1] not in relative_subdirs[index + 1].parents:
        return True
      else:
        return False

    filtered_subdirs: List[Tuple[int, Path]] = list(
        filter(_FilterFunc, enumerate(relative_subdirs)))
    # convert from enumerated tuples back into list of Path objects
    if filtered_subdirs != []:
      relative_subdirs = list(list(zip(*filtered_subdirs))[1])
    else:
      relative_subdirs = []

    # convert relative paths to full destination paths
    dst_subdirs = [self.output_dir / path for path in relative_subdirs]
    CreateDirectories(dst_subdirs, is_dry_run=self.is_dry_run)

  def _CopyNoTransform(
      self,
      relative_paths: List[Tuple[Path, Path]],
      relative_to: Path = Path(".")
  ) -> None:

    full_paths = [(relative_to / item[0], self.output_dir / item[1])
                  for item in relative_paths]
    CopyFiles(full_paths, is_dry_run=self.is_dry_run)

  def _CopyWithTransform(
      self,
      path_pairs: List[Tuple[Path, Path]],
      headers_dict: Dict[Union[str, None], str],
      relative_to: Path = Path(".")
  ) -> None:

    script_path = Path(
        "src/tensorflow/lite/micro/tools/make/transform_source.py")

    # transform all source and header files
    for relative_paths in path_pairs:
      dst_path = self.output_dir / relative_paths[1]
      src_path = relative_to / relative_paths[0]
      if relative_paths[1].parts[0] == "examples":
        third_party_headers = headers_dict[relative_paths[1].parts[1]]
        if relative_paths[1].suffix == ".ino":
          is_example_ino = True
          is_example_source = False
        else:
          is_example_ino = False
          is_example_source = True
      else:
        third_party_headers = headers_dict[None]
        is_example_source = False
        is_example_ino = False

      args = "--platform=arduino"
      if is_example_ino:
        args += " --is_example_ino"
      elif is_example_source:
        args += " --is_example_source"
      args += f' --third_party_headers="{third_party_headers}"'
      args += f" < {src_path!s} > {dst_path!s}"
      RunPythonScript(script_path, args=args, is_dry_run=self.is_dry_run)

  def _PatchWithSed(self, patches: List[Tuple[List[Path], List[str]]]) -> None:
    for files, scripts in patches:
      for file in files:
        RunSedScripts(self.output_dir / file,
                      scripts=scripts,
                      is_dry_run=self.is_dry_run)

  def _FixSubDirectories(self) -> None:
    script_path = Path(
        "src/tensorflow/lite/micro/tools/make/fix_arduino_subfolders.py")
    args = str(self.output_dir)
    RunPythonScript(script_path, args, is_dry_run=self.is_dry_run)

  def _MakeZipFile(self) -> None:
    print(f"Creating ZIP file: {self.output_dir!s}.zip")
    shutil.make_archive(base_name=str(self.output_dir),
                        format="zip",
                        root_dir=self.output_dir.parent,
                        base_dir=self.output_dir.name,
                        dry_run=self.is_dry_run,
                        logger=logging.getLogger())

  def _GenerateHeaderList(self, example: Union[str, None],
                          all_path_pairs: List[Tuple[Path, Path]]) -> List[str]:
    result_list: List[str] = []
    for path in all_path_pairs:
      if path[1].suffix != ".h":
        continue
      if example is not None:
        # need the headers for this example
        relative_path = Path("examples") / example
        if relative_path in path[1].parents:
          s = str(path[1])
          result_list.append(s)
      # add third-party headers
      relative_path = Path("src/third_party")
      if relative_path in path[1].parents:
        s = str(relative_path.parts[1] / path[1].relative_to(relative_path))
        result_list.append(s)

    return result_list

  def _GenerateHeadersDict(
      self, all_path_pairs: List[Tuple[Path,
                                       Path]]) -> Dict[Union[str, None], str]:
    headers_dict: Dict[Union[str, None], str] = {
        example: " ".join(
            self._GenerateHeaderList(example, all_path_pairs=all_path_pairs))
        for example in self.examples
    }
    headers_dict[None] = " ".join(
        self._GenerateHeaderList(None, all_path_pairs=all_path_pairs))
    return headers_dict

  def _GenerateBasePathsRelative(
      self, generate_transform_paths: bool) -> List[Tuple[Path, Path]]:

    # generate set of all files from base directory
    all_files = self.base_dir.glob("**/*")
    # filter out directories
    all_files = filter(lambda p: not p.is_dir(), all_files)
    special_path_dict = dict(self.special_base_list)

    # generate relative source/destination pairs
    relative_path_pairs: List[Tuple[Path, Path]] = []
    for file in all_files:
      src_path = Path(file).relative_to(self.base_dir)
      # check if in manifest special base or remove list
      if special_path_dict.get(src_path) is not None:
        continue
      elif src_path in self.remove_list:
        continue
      # tensorflow/ and third_party/ will be subdirectories of src/
      if src_path.parts[0] in ["third_party", "tensorflow"]:
        dst_path = "src" / src_path
      else:
        dst_path = src_path

      # check for .cc to .cpp rename
      # future: apply suffixes_mapping= instead
      if dst_path.suffix == ".cc":
        dst_path = dst_path.with_suffix(".cpp")

      # add new tuple(src,dst) to list
      relative_path_pairs.append((src_path, dst_path))

    # add all manifest special base paths
    for paths in self.special_base_list:
      from_path = paths[0]
      to_path = paths[1]
      # future: apply suffixes_mapping=
      relative_path_pairs.append((from_path, to_path))

    # filter for (non)transformable
    # future: use transform_suffixes=
    xform_suffixes = [".c", ".cc", ".cpp", ".h", ".ino"]
    relative_path_pairs = list(
        filter(
            lambda pair: generate_transform_paths ==
            (pair[0].suffix in xform_suffixes), relative_path_pairs))

    return relative_path_pairs

  def _GenerateRepoPathsRelative(
      self, generate_transform_paths: bool) -> List[Tuple[Path, Path]]:

    relative_path_pairs: List[Tuple[Path, Path]] = []
    if generate_transform_paths:
      # future: download (url=) sections using transform_suffixes= list
      # future: apply suffixes_mapping=
      # for now just return empty list
      pass
    else:
      # future: download (url=) sections not in transform_suffixes= list
      for path in self.add_list:
        # future: apply suffixes_mapping=
        relative_path_pairs.append((path, path))
      for paths in self.special_repo_list:
        from_path = paths[0]
        to_path = paths[1]
        # future: apply suffixes_mapping=
        relative_path_pairs.append((from_path, to_path))

    return relative_path_pairs

  def _RemoveEmptyDirectories(self) -> None:
    paths = self.output_dir.glob("**")
    RemoveEmptyDirectories(paths,
                           root=self.output_dir,
                           is_dry_run=self.is_dry_run)

  #
  # public methods
  #

  def CreateZip(self) -> None:
    """
    Execute all steps to create TFLM Arduino ZIP file
    """
    self._CleanOutputDirectory()
    base_xform_paths = self._GenerateBasePathsRelative(True)
    base_copy_paths = self._GenerateBasePathsRelative(False)
    repo_xform_paths = self._GenerateRepoPathsRelative(True)
    repo_copy_paths = self._GenerateRepoPathsRelative(False)
    all_path_pairs = [
        *base_xform_paths,
        *base_copy_paths,
        *repo_xform_paths,
        *repo_copy_paths,
    ]
    self._CreateOutputDirectories(all_path_pairs)
    headers_dict = self._GenerateHeadersDict(all_path_pairs)
    self._CopyWithTransform(base_xform_paths,
                            headers_dict=headers_dict,
                            relative_to=self.base_dir)
    self._CopyWithTransform(repo_xform_paths, headers_dict=headers_dict)
    self._CopyNoTransform(base_copy_paths, relative_to=self.base_dir)
    self._CopyNoTransform(repo_copy_paths)
    self._PatchWithSed(self.patch_sed_list)
    self._FixSubDirectories()
    self._RemoveEmptyDirectories()
    self._MakeZipFile()


if __name__ == "__main__":
  generator = ArduinoProjectGenerator()
  generator.CreateZip()
