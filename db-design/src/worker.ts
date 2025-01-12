declare var self: Worker;

import logger from "@/utils/logger";
import { pgQuery } from "./postgres";
import { ZodError, type ZodRawShape } from "zod";
import { mongoQuery } from "./mongo";
import {
  mongoGame,
  mongoGameOpening,
  mongoGameToUser,
  mongoTimeControl,
  mongoUser,
} from "./mongo/schema";
import {
  pgGame,
  pgGameOpening,
  pgGameToUser,
  pgTimeControl,
  pgUser,
  type pgTable,
} from "./postgres/schema";

async function findGreatestDate() {
  const promises = tables.map(({ mongo }) =>
    mongoQuery(async () => {
      const lastDocument = await mongo.aggregate([
        { $sort: { created_at: -1 } },
        { $limit: 1 },
      ]);
      return lastDocument[0]?.created_at as Date | undefined;
    }),
  );

  const results = await Promise.allSettled(promises);

  const filtered = results
    .filter(result => result.status === "fulfilled")
    .map(result => result.value)
    .filter(value => value !== undefined)
    .map(date => date.getTime());

  if (filtered.length === 0) {
    logger.info("No data inserted yet. Starting from scratch.");
    return null;
  }

  const greatestDate = new Date(Math.max(...filtered));
  logger.info("Starting from", greatestDate.toISOString());
  return greatestDate;
}

async function sleep(ms: number) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function fetchData<T extends ZodRawShape>(
  table: pgTable<T>,
  lastDate: Date | null,
) {
  if (lastDate === null) {
    const query = "SELECT * FROM __table__";
    return pgQuery(table, query);
  }

  const query =
    "SELECT * FROM __table__ WHERE DATE_TRUNC('second', created_at) > $1";
  return pgQuery(table, query, [lastDate]);
}

export default async function main(lastGreatestDate: Date | null) {
  if (lastGreatestDate === null) {
    lastGreatestDate = await findGreatestDate();
  }

  let currentGreatestDate = lastGreatestDate;

  const tablePromises = tables.map(async ({ pg, mongo, table }) => {
    const rows = await fetchData(pg as pgTable<ZodRawShape>, lastGreatestDate);

    if (rows.length === 0) {
      logger.info(`No rows found for "${table}, skipping"`);
      return;
    }

    rows.forEach(row => {
      const r = row as { created_at: Date };
      if (currentGreatestDate === null) {
        currentGreatestDate = r.created_at;
      } else if (r.created_at > currentGreatestDate) {
        currentGreatestDate = r.created_at;
      }
    });

    logger.info(`Retrieved ${rows.length} rows from Postgres for "${table}"`);

    await mongoQuery(async () => {
      // @ts-ignore
      const mongoInsertPromises = rows.map(row => mongo.create(row));
      const insertResults = await Promise.allSettled(mongoInsertPromises);

      let successful = 0;

      insertResults.forEach(result => {
        if (result.status === "rejected") {
          logger.error(`Failed to save ${table} to Mongo:`, result.reason);
        } else successful++;
      });

      logger.info(`Saved ${successful} rows to Mongo for "${table}"`);
    });
  });

  const results = await Promise.allSettled(tablePromises);
  results.forEach(result => {
    if (result.status === "rejected") logger.error(result.reason);
  });

  return currentGreatestDate;
}

self.onmessage = async () => {
  try {
    logger.info("Running worker.ts");

    let lastGreatestDate: Date | null = null;

    while (true) {
      lastGreatestDate = await main(lastGreatestDate);
      const s = MINUTE;
      logger.info("Sleeping for", s / SECOND, "s");
      await sleep(s);
    }
  } catch (e) {
    if (e instanceof ZodError) {
      logger.error(e.cause);
      logger.error("ZodError");
      e.errors.forEach(error => {
        logger.error(`  Path: ${error.path.join(".")}`);
        logger.error(`  Message: ${error.message}`);
      });
    } else logger.error(e);
  }
};

const tables = Object.freeze([
  { pg: pgUser, mongo: mongoUser, table: "user" },
  { pg: pgGameOpening, mongo: mongoGameOpening, table: "game_opening" },
  { pg: pgTimeControl, mongo: mongoTimeControl, table: "time_control" },
  { pg: pgGame, mongo: mongoGame, table: "game" },
  { pg: pgGameToUser, mongo: mongoGameToUser, table: "game_to_user" },
] as const);

const SECOND = 1000;
const MINUTE = SECOND * 60;
