from os import sep
from guadalinex import trunk_dir, tags_dir, metapkgs_dir, apps_dir, gcs_dir

def split_guada_path(path):
    pieces = path.split(sep)
    if pieces[0] == apps_dir:
	if pieces[2] == trunk_dir:
	    return (None, sep.join(pieces[3:]))
	elif pieces[2] == tags_dir:
	    return (sep.join(pieces[0:3]), sep.join(pieces[3:]))
    elif pieces[0] == metapkgs_dir: 
	    return (sep.join(pieces[0:2]), sep.join(pieces[2:]))
    else:
    	return None
