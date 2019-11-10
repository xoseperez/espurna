<template>
    <table class="dataTable">
        <thead>
            <tr>
                <th v-for="field in columns" :key="field.key"
                    :class="fieldClasses(field)"
                    @click="() => toggleFilter(field)"
                    @mouseover="hover=field.key" @mouseleave="hover=''">
                    {{field.title}}
                    <span v-if="orderBy === field.key" class="order-arrow">{{getOrderIcon(field)}}</span>
                </th>
            </tr>
        </thead>
        <tbody>
            <tr v-for="row in rows" :key="getUniqueKey(row)">
                <td v-for="field in columns" :key="getUniqueKey(row)+'_'+field.key"
                    :class="fieldClasses(field)">
                    {{row[field.key]}}
                </td>
            </tr>
        </tbody>
        <tfoot>
            <tr>
                <td :colspan="columns.length"><!-- todo pagination --></td>
            </tr>
        </tfoot>
    </table>
</template>

<script>
    const sort_by = (field, reverse, primer) => {

        const key = primer ?
            function (x) {
                return primer(x[field])
            } :
            function (x) {
                return x[field]
            };

        reverse = !reverse ? 1 : -1;

        return function (a, b) {
            a = key(a);
            b = key(b);
            return reverse * ((a > b) - (b > a));
        }
    };

    export default {
        props: {
            columns: {
                type: Object,
                required: true
            },
            uniqueKey: {
                type: [String, Function],
                required: true
            },
            value: {
                type: Array,
                required: true
            },
            search: {
                type: String,
                required: false
            }
        },
        data() {
            return {
                orderBy: null,
                order: false,
                hover: ""
            }
        },
        computed: {
            rows() {
                if (this.orderBy) {
                    let val = {...this.value};
                    //TODO add data type in primer
                    return val.sort(sort_by(this.orderBy, this.order));
                }
                return this.value;
            }
        },
        methods: {
            getUniqueKey(row) {
                return typeof this.uniqueKey === 'string' ? row[this.uniqueKey] : this.uniqueKey.call(this, row);
            },
            fieldClasses(field) {
                let classes = [];

                if (field.class) {
                    classes.push(field.class);
                }
                if (field.align) {
                    classes.push('align-' + field.align);
                }
                if (this.orderBy === field.key) {
                    classes.push(this.order ? 'order-asc' : 'order-desc');
                }
                return classes;
            },
            newFieldOrder(field) {
                return "asc" in field ? !!field.asc : ("desc" in field ? !field.desc : "order" in field ? field.order.match(/asc/i) : true)
            },
            getOrderIcon(field) {
                let hover = this.hover === field.key;
                let current = this.orderBy === field.key;
                if (current || hover) {
                    let order = current ? !!(this.order * (-1 + hover)) : this.newFieldOrder(field);

                    return order ? '▾' : '▴';
                }
                return ""
            },
            toggleFilter(field) {
                if (field.key === this.orderBy) {
                    this.order = !this.order;
                } else {
                    this.orderBy = field.key;
                    this.order = this.newFieldOrder(field);
                }
            }
        }
    }
</script>

<style lang="less">
    .dataTable {
        width: 100%;
        border-spacing: 0;
        font-family: "Roboto", sans-serif;
        letter-spacing: .1px;
        line-height: 22px;

        .align-left {
            text-align: left;
        }

        .align-center {
            text-align: center;
        }

        .align-right {
            text-align: right;
        }

        thead tr th {
            border-bottom: 1px solid rgba(0, 0, 0, 0.12);
            color: rgba(0, 0, 0, 0.54);
            cursor: pointer;
            pointer-events: auto;
            font-size: 12px;
            height: 42px;
            padding: 0 16px;

            &:hover {
                color: rgba(0, 0, 0, 0.87);
            }
        }

        tbody {
            tr {
                &:hover {
                    background: #f5f5f5;
                }

                &:not(:last-child) td {
                    border-bottom: 1px solid rgba(0, 0, 0, 0.12);
                    font-size: 14px;
                    height: 42px;
                }
            }
        }
    }
</style>