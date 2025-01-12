BEGIN;

CREATE SCHEMA raw;

CREATE TABLE raw.staging (
    load_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP PRIMARY KEY,
    event_data JSONB NOT NULL
);

CREATE TABLE raw.load_tracker (
    current_offset INT PRIMARY KEY
);

CREATE TABLE raw.processed_tracker (
    current_processed INT PRIMARY KEY
);

INSERT INTO raw.load_tracker (current_offset) VALUES (0);
INSERT INTO raw.processed_tracker (current_processed) VALUES (0);

CREATE OR REPLACE PROCEDURE raw.stage_data()
LANGUAGE plpgsql
AS $$
DECLARE
    batch_limit INT := 100;

    json_data JSONB;
    event_data JSONB;
    total_objects INT;

    current_offset INT;
    stop_offset INT;
BEGIN
    RAISE NOTICE 'STAGE DATA: Starting data staging process.';

    json_data := COALESCE(
        pg_read_file('/home/data.json')::TEXT::JSONB,
        '[]'::JSONB
    );
    total_objects := jsonb_array_length(json_data);

    RAISE NOTICE 'STAGE DATA: JSON data loaded with length: %', total_objects;

    SELECT raw.load_tracker.current_offset INTO current_offset
    FROM raw.load_tracker
    ORDER BY raw.load_tracker.current_offset LIMIT 1;

    RAISE NOTICE 'STAGE DATA: Current offset from load_tracker: %', current_offset;

    IF current_offset >= total_objects THEN
        RAISE NOTICE 'STAGE DATA: All objects have already been loaded.';
        RETURN;
    END IF;

    stop_offset := LEAST(current_offset + batch_limit - 1, total_objects - 1);
    event_data := '[]'::JSONB;

    RAISE NOTICE 'STAGE DATA: Processing from offset % to %', current_offset, stop_offset;

    FOR i IN current_offset..stop_offset LOOP
        event_data := jsonb_insert(event_data, '{0}', json_data->(i), true);
    END LOOP;

    INSERT INTO raw.staging (event_data) VALUES (event_data);
    UPDATE raw.load_tracker SET current_offset = stop_offset + 1;

    RAISE NOTICE 'STAGE DATA: % objects loaded.', stop_offset - current_offset + 1;
END;
$$;

CREATE SCHEMA processed;

CREATE TABLE processed.user (
    id TEXT PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE processed.game_opening (
    eco TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE processed.time_control (
    id SERIAL PRIMARY KEY,
    min INT NOT NULL,
    sec INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TYPE processed.victory_type AS ENUM (
    'draw',
    'mate',
    'resign',
    'outoftime'
);

CREATE TABLE processed.game (
    id TEXT PRIMARY KEY,
    is_rated BOOLEAN NOT NULL,
    no_turns INT NOT NULL,
    victory_type processed.victory_type NOT NULL,
    moves TEXT NOT NULL,
    time_ctrl_id INT NOT NULL REFERENCES processed.time_control(id),
    opening_eco TEXT NOT NULL REFERENCES processed.game_opening(eco),
    opening_ply INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TYPE processed.user_color AS ENUM (
    'white',
    'black',
    'draw'
);

CREATE TABLE processed.game_to_user (
    game_id TEXT REFERENCES processed.game(id),
    user_id TEXT REFERENCES processed.user(id),
    user_rating INT NOT NULL,
    user_color processed.user_color NOT NULL,
    user_won BOOLEAN NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (game_id, user_id)
);

CREATE OR REPLACE FUNCTION get_or_insert_time_control(time_control_str TEXT)
RETURNS INT AS $$
DECLARE
    min_part INT;
    sec_part INT;
    time_control_id INT;
BEGIN
    min_part := (split_part(time_control_str, '+', 1))::INT;
    sec_part := (split_part(time_control_str, '+', 2))::INT;

    SELECT id INTO time_control_id
    FROM processed.time_control
    WHERE min = min_part AND sec = sec_part;

    IF time_control_id IS NULL THEN
        INSERT INTO processed.time_control (min, sec)
        VALUES (min_part, sec_part)
        RETURNING id INTO time_control_id;
    END IF;

    RETURN time_control_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION game_exists(game_id TEXT)
RETURNS BOOLEAN AS $$
DECLARE
    existing_game_id TEXT;
BEGIN
    SELECT id INTO existing_game_id
    FROM processed.game
    WHERE id = game_id;

    RETURN CASE WHEN existing_game_id IS NULL THEN false ELSE true END;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE PROCEDURE insert_user_if_not_exists(user_id TEXT)
LANGUAGE plpgsql
AS $$
DECLARE
    existing_user_id TEXT;
BEGIN
    SELECT id INTO existing_user_id
    FROM processed.user
    WHERE id = user_id;

    IF existing_user_id IS NULL THEN
        INSERT INTO processed.user (id) VALUES (user_id);
    END IF;
END;
$$;

CREATE OR REPLACE PROCEDURE insert_opening_if_not_exists(op_eco TEXT, op_name TEXT)
LANGUAGE plpgsql
AS $$
DECLARE
    existing_op_eco TEXT;
BEGIN
    SELECT eco INTO existing_op_eco
    FROM processed.game_opening
    WHERE eco = op_eco;

    IF existing_op_eco IS NULL THEN
        INSERT INTO processed.game_opening (eco, name) VALUES (op_eco, op_name);
    END IF;
END;
$$;

CREATE OR REPLACE PROCEDURE insert_new_game(
    id TEXT,
    is_rated BOOLEAN,
    no_turns INT,
    victory_type processed.victory_type,
    moves TEXT,
    time_ctrl_id INT,
    opening_eco TEXT,
    opening_ply INT
)
LANGUAGE plpgsql
AS $$
BEGIN
    INSERT INTO processed.game (
        id,
        is_rated,
        no_turns,
        victory_type,
        moves,
        time_ctrl_id,
        opening_eco,
        opening_ply
    ) VALUES (
        id,
        is_rated,
        no_turns,
        victory_type,
        moves,
        time_ctrl_id,
        opening_eco,
        opening_ply
    );
END;
$$;

CREATE OR REPLACE PROCEDURE insert_game_to_user(
    game_id TEXT,
    user_id TEXT,
    user_rating INT,
    user_color processed.user_color,
    user_won BOOLEAN
)
LANGUAGE plpgsql
AS $$
BEGIN
    INSERT INTO processed.game_to_user (
        game_id,
        user_id,
        user_rating,
        user_color,
        user_won
    ) VALUES (
        game_id,
        user_id,
        user_rating,
        user_color,
        user_won
    );
END;
$$;

CREATE OR REPLACE PROCEDURE raw.process_data()
LANGUAGE plpgsql
AS $$
DECLARE
    proc_current_processed INT;
    event_data JSONB;
    object JSONB;
BEGIN

    RAISE NOTICE 'PROCESS DATA: Starting process_data procedure.';

    SELECT raw.processed_tracker.current_processed INTO proc_current_processed
    FROM raw.processed_tracker
    ORDER BY raw.processed_tracker.current_processed LIMIT 1;

    RAISE NOTICE 'PROCESS DATA: Current processed index: %', proc_current_processed;

    SELECT raw.staging.event_data INTO event_data
    FROM raw.staging
    OFFSET proc_current_processed LIMIT 1;

    IF event_data IS NULL THEN
        RAISE NOTICE 'PROCESS DATA: No unprocessed event data found.';
        RETURN;
    END IF;

    FOR object IN SELECT jsonb_array_elements(event_data) LOOP
    DECLARE
        game_id TEXT := object->>'id';
        is_rated BOOLEAN := (object->>'rated')::BOOLEAN;
        no_turns INT := (object->>'turns')::INT;
        victory_type processed.victory_type := (object->>'victory_status')::processed.victory_type;
        winner TEXT := object->>'winner';
        increment_code TEXT := object->>'increment_code';
        white_id TEXT := object->>'white_id';
        white_rating INT := (object->>'white_rating')::INT;
        black_id TEXT := object->>'black_id';
        black_rating INT := (object->>'black_rating')::INT;
        moves TEXT := object->>'moves';
        opening_eco TEXT := object->>'opening_eco';
        opening_name TEXT := object->>'opening_name';
        opening_ply INT := (object->>'opening_ply')::INT;

        time_ctrl_id INT;
        proc_game_exists BOOLEAN;
    BEGIN
        SELECT game_exists(game_id) INTO proc_game_exists;

        if proc_game_exists THEN
            RAISE EXCEPTION 'PROCESS DATA: ERROR Game already exists: %', game_id;
            RETURN;
        END IF;

        CALL insert_user_if_not_exists(white_id);

        CALL insert_user_if_not_exists(black_id);

        CALL insert_opening_if_not_exists(opening_eco, opening_name);

        SELECT get_or_insert_time_control(increment_code) INTO time_ctrl_id;

        CALL insert_new_game(
            game_id,
            is_rated,
            no_turns,
            victory_type,
            moves,
            time_ctrl_id,
            opening_eco,
            opening_ply
        );

        CALL insert_game_to_user(
            game_id,
            white_id,
            white_rating,
            'white',
            CASE WHEN winner = 'white' THEN true ELSE false END
        );

        CALL insert_game_to_user(
            game_id,
            black_id,
            black_rating,
           'black',
            CASE WHEN winner = 'black' THEN true ELSE false END
        );
    END;
    END LOOP;

    RAISE NOTICE 'PROCESS DATA: Updating processed tracker from % to %', proc_current_processed, proc_current_processed + 1;

    UPDATE raw.processed_tracker
    SET current_processed = proc_current_processed + 1;

    RAISE NOTICE 'PROCESS DATA: Process completed successfully.';
END;
$$;

COMMIT;

CREATE EXTENSION pg_cron;

SELECT cron.schedule('* * * * *', $$
    CALL raw.stage_data();
$$);

SELECT cron.schedule('* * * * *', $$
    CALL raw.process_data();
$$);
