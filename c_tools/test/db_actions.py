#!/usr/bin/env python3
# This is a file test for the ai agent from the tui
"""
Universal CLI tool for SQLite CRUD operations
"""

import sqlite3
import sys
import argparse
import json
from pathlib import Path
from typing import Optional, List, Dict, Any


class SQLiteDB:
    """SQLite database manager"""
    
    def __init__(self, db_path: str):
        self.db_path = db_path
        self.conn = None
        self.cursor = None
    
    def connect(self):
        """Connect to database"""
        self.conn = sqlite3.connect(self.db_path)
        self.cursor = self.conn.cursor()
    
    def disconnect(self):
        """Disconnect from database"""
        if self.conn:
            self.conn.close()
    
    def create_table(self, table_name: str, schema: str) -> bool:
        """Create a new table"""
        try:
            self.cursor.execute(f"CREATE TABLE IF NOT EXISTS {table_name} ({schema})")
            self.conn.commit()
            print(f" Table '{table_name}' created successfully")
            return True
        except sqlite3.Error as e:
            print(f" Error creating table: {e}")
            return False
    
    def insert(self, table_name: str, columns: List[str], values: List[Any]) -> bool:
        """Insert a single row"""
        try:
            placeholders = ",".join(["?" for _ in values])
            cols = ",".join(columns)
            query = f"INSERT INTO {table_name} ({cols}) VALUES ({placeholders})"
            self.cursor.execute(query, values)
            self.conn.commit()
            print(f" Row inserted into '{table_name}'")
            return True
        except sqlite3.Error as e:
            print(f" Error inserting row: {e}")
            return False
    
    def select(self, table_name: str, where: Optional[str] = None, 
               limit: Optional[int] = None) -> List[Dict[str, Any]]:
        """Select rows from table"""
        try:
            query = f"SELECT * FROM {table_name}"
            if where:
                query += f" WHERE {where}"
            if limit:
                query += f" LIMIT {limit}"
            
            self.cursor.execute(query)
            columns = [description[0] for description in self.cursor.description]
            rows = self.cursor.fetchall()
            
            results = [dict(zip(columns, row)) for row in rows]
            return results
        except sqlite3.Error as e:
            print(f" Error selecting rows: {e}")
            return []
    
    def update(self, table_name: str, set_clause: str, where: str) -> bool:
        """Update rows in table"""
        try:
            query = f"UPDATE {table_name} SET {set_clause} WHERE {where}"
            self.cursor.execute(query)
            self.conn.commit()
            print(f" {self.cursor.rowcount} row(s) updated in '{table_name}'")
            return True
        except sqlite3.Error as e:
            print(f" Error updating rows: {e}")
            return False
    
    def delete(self, table_name: str, where: str) -> bool:
        """Delete rows from table"""
        try:
            query = f"DELETE FROM {table_name} WHERE {where}"
            self.cursor.execute(query)
            self.conn.commit()
            print(f" {self.cursor.rowcount} row(s) deleted from '{table_name}'")
            return True
        except sqlite3.Error as e:
            print(f" Error deleting rows: {e}")
            return False
    
    def list_tables(self) -> List[str]:
        """List all tables in database"""
        try:
            self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
            tables = [row[0] for row in self.cursor.fetchall()]
            return tables
        except sqlite3.Error as e:
            print(f" Error listing tables: {e}")
            return []
    
    def drop_table(self, table_name: str) -> bool:
        """Drop a table"""
        try:
            self.cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
            self.conn.commit()
            print(f" Table '{table_name}' dropped")
            return True
        except sqlite3.Error as e:
            print(f" Error dropping table: {e}")
            return False
    
    def export_json(self, table_name: str, output_file: str) -> bool:
        """Export table to JSON"""
        try:
            rows = self.select(table_name)
            with open(output_file, 'w') as f:
                json.dump(rows, f, indent=2, default=str)
            print(f" Table '{table_name}' exported to '{output_file}'")
            return True
        except Exception as e:
            print(f" Error exporting to JSON: {e}")
            return False


def main():
    """Main CLI entry point"""
    parser = argparse.ArgumentParser(
        description="Universal CLI tool for SQLite CRUD operations",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python db_actions.py -d test.db create-table users "id INTEGER PRIMARY KEY, name TEXT, email TEXT"
  python db_actions.py -d test.db insert users "name,email" "John,john@example.com"
  python db_actions.py -d test.db select users
  python db_actions.py -d test.db select users -w "id=1"
  python db_actions.py -d test.db update users "name='Jane'" "id=1"
  python db_actions.py -d test.db delete users "id=1"
  python db_actions.py -d test.db list-tables
  python db_actions.py -d test.db drop-table users
  python db_actions.py -d test.db export-json users output.json
        """
    )
    
    parser.add_argument("-d", "--database", required=True, help="Path to SQLite database")
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")
    
    # Create table
    create_parser = subparsers.add_parser("create-table", help="Create a new table")
    create_parser.add_argument("table_name", help="Table name")
    create_parser.add_argument("schema", help="Table schema (e.g., 'id INTEGER PRIMARY KEY, name TEXT')")
    
    # Insert
    insert_parser = subparsers.add_parser("insert", help="Insert a row")
    insert_parser.add_argument("table_name", help="Table name")
    insert_parser.add_argument("columns", help="Comma-separated column names")
    insert_parser.add_argument("values", help="Comma-separated values")
    
    # Select
    select_parser = subparsers.add_parser("select", help="Select rows")
    select_parser.add_argument("table_name", help="Table name")
    select_parser.add_argument("-w", "--where", help="WHERE clause")
    select_parser.add_argument("-l", "--limit", type=int, help="LIMIT rows")
    
    # Update
    update_parser = subparsers.add_parser("update", help="Update rows")
    update_parser.add_argument("table_name", help="Table name")
    update_parser.add_argument("set_clause", help="SET clause (e.g., 'name=\"John\"')")
    update_parser.add_argument("where", help="WHERE clause")
    
    # Delete
    delete_parser = subparsers.add_parser("delete", help="Delete rows")
    delete_parser.add_argument("table_name", help="Table name")
    delete_parser.add_argument("where", help="WHERE clause")
    
    # List tables
    subparsers.add_parser("list-tables", help="List all tables")
    
    # Drop table
    drop_parser = subparsers.add_parser("drop-table", help="Drop a table")
    drop_parser.add_argument("table_name", help="Table name")
    
    # Export JSON
    export_parser = subparsers.add_parser("export-json", help="Export table to JSON")
    export_parser.add_argument("table_name", help="Table name")
    export_parser.add_argument("output_file", help="Output JSON file path")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    # Initialize database
    db = SQLiteDB(args.database)
    db.connect()
    
    try:
        if args.command == "create-table":
            db.create_table(args.table_name, args.schema)
        
        elif args.command == "insert":
            columns = [c.strip() for c in args.columns.split(",")]
            values = [v.strip() for v in args.values.split(",")]
            db.insert(args.table_name, columns, values)
        
        elif args.command == "select":
            rows = db.select(args.table_name, args.where, args.limit)
            if rows:
                print(json.dumps(rows, indent=2, default=str))
            else:
                print("No rows found")
        
        elif args.command == "update":
            db.update(args.table_name, args.set_clause, args.where)
        
        elif args.command == "delete":
            db.delete(args.table_name, args.where)
        
        elif args.command == "list-tables":
            tables = db.list_tables()
            if tables:
                print("Tables:")
                for table in tables:
                    print(f"  - {table}")
            else:
                print("No tables found")
        
        elif args.command == "drop-table":
            db.drop_table(args.table_name)
        
        elif args.command == "export-json":
            db.export_json(args.table_name, args.output_file)
    
    finally:
        db.disconnect()


if __name__ == "__main__":
    main()