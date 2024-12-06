from schema import And, Optional, Schema

def validate_ids(field):
    """
    Ensure that 1) either seq_id or id is provided,
    and 2) if an object is migratable, collection_id has been set.
    """
    if 'seq_id' not in field and 'id' not in field:
        raise ValueError('Either id (bit-encoded) or seq_id must be provided.')

    if field.get("migratable") is True and 'seq_id' in field and 'collection_id' not in field:
        raise ValueError('If an entity is migratable, it must have a collection_id')

    return field

LBDatafile_schema = Schema(
    {
        Optional('type'): And(str, "LBDatafile", error="'LBDatafile' must be chosen."),
        Optional('metadata'): {
            Optional('type'): And(str, "LBDatafile", error="'LBDatafile' must be chosen."),
            Optional('rank'): int,
            Optional('shared_node'): {
                'id': int,
                'size': int,
                'rank': int,
                'num_nodes': int,
            },
            Optional('phases'): {
                Optional('count'): int,
                'skipped': {
                    'list': [int],
                    'range': [[int]],
                },
                'identical_to_previous': {
                    'list': [int],
                    'range': [[int]],
                },
            },
            Optional('attributes'): dict
        },
        'phases': [
            {
                'id': int,
                'tasks': [
                    {
                        'entity': And({
                            Optional('collection_id'): int,
                            'home': int,
                            Optional('id'): int,
                            Optional('seq_id'): int,
                            Optional('index'): [int],
                            'type': str,
                            'migratable': bool,
                            Optional('objgroup_id'): int
                        }, validate_ids),
                        'node': int,
                        'resource': str,
                        Optional('subphases'): [
                            {
                                'id': int,
                                'time': float,
                            }
                        ],
                        'time': float,
                        Optional('user_defined'): dict,
                        Optional('attributes'): dict
                    },
                ],
                Optional('communications'): [
                    {
                        'type': str,
                        'to': And({
                            'type': str,
                            Optional('id'): int,
                            Optional('seq_id'): int,
                            Optional('home'): int,
                            Optional('collection_id'): int,
                            Optional('migratable'): bool,
                            Optional('index'): [int],
                            Optional('objgroup_id'): int,
                        }, validate_ids),
                        'messages': int,
                        'from': And({
                            'type': str,
                            Optional('id'): int,
                            Optional('seq_id'): int,
                            Optional('home'): int,
                            Optional('collection_id'): int,
                            Optional('migratable'): bool,
                            Optional('index'): [int],
                            Optional('objgroup_id'): int,
                        }, validate_ids),
                        'bytes': float
                    }
                ],
                Optional('user_defined'): dict,
                Optional('lb_iterations'): [
                    {
                        'id': int,
                        'tasks': [
                            {
                                'entity': And({
                                    Optional('collection_id'): int,
                                    'home': int,
                                    Optional('id'): int,
                                    Optional('seq_id'): int,
                                    Optional('index'): [int],
                                    'type': str,
                                    'migratable': bool,
                                    Optional('objgroup_id'): int
                                }, validate_ids),
                                'node': int,
                                'resource': str,
                                Optional('subphases'): [
                                    {
                                        'id': int,
                                        'time': float,
                                    }
                                ],
                                'time': float,
                                Optional('user_defined'): dict,
                                Optional('attributes'): dict
                            },
                        ],
                        Optional('communications'): [
                            {
                                'type': str,
                                'to': And({
                                    'type': str,
                                    Optional('id'): int,
                                    Optional('seq_id'): int,
                                    Optional('home'): int,
                                    Optional('collection_id'): int,
                                    Optional('migratable'): bool,
                                    Optional('index'): [int],
                                    Optional('objgroup_id'): int,
                                }, validate_ids),
                                'messages': int,
                                'from': And({
                                    'type': str,
                                    Optional('id'): int,
                                    Optional('seq_id'): int,
                                    Optional('home'): int,
                                    Optional('collection_id'): int,
                                    Optional('migratable'): bool,
                                    Optional('index'): [int],
                                    Optional('objgroup_id'): int,
                                }, validate_ids),
                                'bytes': float
                            }
                        ],
                        Optional('user_defined'): dict
                    }
                ]
            },
        ]
    }
)
