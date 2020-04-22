import {mount} from "@vue/test-utils";
import Input from "../../src/components/Input";
import Vue from "vue";

describe("Input", () => {
    let values = {};

    const form = new Vue({data: {values}});

    const provide = () => {
        return {
            $form: () => ({
                values: form.values
            })
        };
    };

    it("should render correctly", () => {
        const input = mount(Input, {
            propsData: {
                type: "text", name: "test1"
            }
        });

        expect(input.exists()).toBeTruthy();

        const in1 = input.find("input");

        expect(in1.element.type).toBe("text");
    });

    it("should have a default value", () => {

        let input = mount(Input, {
            propsData: {
                type: "text", name: "test1", default: "default"
            },
            provide
        });

        expect(input.vm.val).toBe("default");
    });


    it("should work with v-model", () => {

        let input = mount(Input, {
            propsData: {
                type: "text", value: "test"
            },
        });

        expect(input.vm.val).toBe("test");


        const in1 = input.find("input");
        in1.element.value = "Hello world";
        in1.trigger("input");

        expect(input.emitted().input).toBeTruthy();
        expect(input.emitted().input.length).toBe(1);
        expect(input.emitted().input[0]).toEqual(["Hello world"]);
    });

    it("should emit input when its internal value changes", () => {
        const input = mount(Input, {
            propsData: {
                type: "text", name: "test1"
            },
            provide
        });

        const in1 = input.find("input");
        in1.setValue("Hello world");

        expect(input.emitted().input).toBeTruthy();
        expect(input.emitted().input.length).toBe(1);
        expect(input.emitted().input[0]).toEqual(["Hello world"]);
    });

    it("should update its value for text", async () => {
        const input = mount(Input, {
            propsData: {
                type: "text", name: "test1"
            },
            provide
        });

        const in1 = input.find("input");
        in1.element.value = "Hello world";
        in1.trigger("input");

        await Vue.nextTick();

        expect(input.vm.val).toBe("Hello world");
        expect(values.test1).toBe("Hello world");
    });

    it("should update its value for switch", async () => {
        const
            input = mount(Input, {
                propsData: {
                    type: "switch", name: "test3"
                },
                provide
            });

        const in3 = input.find("label");
        in3.trigger("click");

        await Vue.nextTick();

        expect(input.vm.val).toBe(true);
        expect(values.test3).toBe(true);

        in3.trigger("click");

        await Vue.nextTick();

        expect(input.vm.val).toBe(false);
        expect(values.test3).toBe(false);

    });

    it("should update its value for select", async () => {
        const input = mount(Input, {
            propsData: {
                type: "select", name: "test2", options: ["1", "2", "3"]
            },
            provide
        });

        const in2 = input.find("select");
        in2.element.value = 1;
        in2.trigger("change");

        await Vue.nextTick();

        expect(input.vm.val).toBe(1);
        expect(values.test2).toBe(1);
    });

});
